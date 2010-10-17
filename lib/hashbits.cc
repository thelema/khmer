#include "hashtable.hh"
#include "hashbits.hh"
#include "parsers.hh"

using namespace std;
using namespace khmer;

void Hashbits::save(std::string outfilename)
{
  assert(_counts[0]);

  unsigned int save_ksize = _ksize;
  unsigned long long save_tablesize;

  ofstream outfile(outfilename.c_str(), ios::binary);

  outfile.write((const char *) &save_ksize, sizeof(save_ksize));

  for (unsigned int i = 0; i < n_tables; i++) {
    save_tablesize = _tablesizes[i];
    unsigned long long tablebytes = save_tablesize / 8 + 1;

    outfile.write((const char *) &save_tablesize, sizeof(save_tablesize));

    outfile.write((const char *) _counts[i], tablebytes);
  }
  outfile.close();
}

void Hashbits::load(std::string infilename)
{
  if (_counts) {
    for (unsigned int i = 0; i < n_tables; i++) {
      delete _counts[i]; _counts[i] = NULL;
    }
    delete _counts; _counts = NULL;
  }
  _tablesizes.clear();
  
  unsigned int save_ksize = 0;
  unsigned long long save_tablesize = 0;

  ifstream infile(infilename.c_str(), ios::binary);
  infile.read((char *) &save_ksize, sizeof(save_ksize));
  _ksize = (WordLength) save_ksize;

  _counts = new BoundedCounterType*[n_tables];
  for (unsigned int i = 0; i < n_tables; i++) {
    HashIntoType tablesize;
    unsigned long long tablebytes;

    infile.read((char *) &save_tablesize, sizeof(save_tablesize));

    tablesize = (HashIntoType) save_tablesize;
    _tablesizes.push_back(tablesize);

    tablebytes = tablesize / 8 + 1;
    _counts[i] = new BoundedCounterType[tablebytes];

    unsigned long long loaded = 0;
    while (loaded != tablebytes) {
      infile.read((char *) _counts[i], tablebytes - loaded);
      loaded += infile.gcount();	// do I need to do this loop?
    }
  }
  infile.close();
}

//////////////////////////////////////////////////////////////////////
// graph stuff

ReadMaskTable * Hashbits::filter_file_connected(const std::string &est,
                                                 const std::string &readsfile,
                                                 unsigned int total_reads)
{
   unsigned int read_num = 0;
   unsigned int n_kept = 0;
   unsigned long long int cluster_size;
   ReadMaskTable * readmask = new ReadMaskTable(total_reads);
   IParser* parser = IParser::get_parser(readsfile.c_str());


   std::string first_kmer = est.substr(0, _ksize);
   SeenSet keeper;
   calc_connected_graph_size(first_kmer.c_str(),
                             cluster_size,
                             keeper);

   while(!parser->is_complete())
   {
      std::string seq = parser->get_next_read().seq;

      if (readmask->get(read_num))
      {
         bool keep = false;

         HashIntoType h = 0, r = 0, kmer;
         kmer = _hash(seq.substr(0, _ksize).c_str(), _ksize, h, r);
         kmer = uniqify_rc(h, r);

         SeenSet::iterator i = keeper.find(kmer);
         if (i != keeper.end()) {
            keep = true;
         }

         if (!keep) {
            readmask->set(read_num, false);
         } else {
            n_kept++;
         }
      }

      read_num++;
   }

   return readmask;
}

void Hashbits::calc_connected_graph_size(const HashIntoType kmer_f,
					  const HashIntoType kmer_r,
					  unsigned long long& count,
					  SeenSet& keeper,
					  const unsigned long long threshold)
const
{
  HashIntoType kmer = uniqify_rc(kmer_f, kmer_r);
  const BoundedCounterType val = get_count(kmer);

  if (val == 0) {
    return;
  }

  // have we already seen me? don't count; exit.
  SeenSet::iterator i = keeper.find(kmer);
  if (i != keeper.end()) {
    return;
  }

  // keep track of both seen kmers, and counts.
  keeper.insert(kmer);
  count += 1;

  // are we past the threshold? truncate search.
  if (threshold && count >= threshold) {
    return;
  }

  // otherwise, explore in all directions.

  // NEXT.

  HashIntoType f, r;
  const unsigned int rc_left_shift = _ksize*2 - 2;

  f = ((kmer_f << 2) & bitmask) | twobit_repr('A');
  r = kmer_r >> 2 | (twobit_comp('A') << rc_left_shift);
  calc_connected_graph_size(f, r, count, keeper, threshold);

  f = ((kmer_f << 2) & bitmask) | twobit_repr('C');
  r = kmer_r >> 2 | (twobit_comp('C') << rc_left_shift);
  calc_connected_graph_size(f, r, count, keeper, threshold);

  f = ((kmer_f << 2) & bitmask) | twobit_repr('G');
  r = kmer_r >> 2 | (twobit_comp('G') << rc_left_shift);
  calc_connected_graph_size(f, r, count, keeper, threshold);

  f = ((kmer_f << 2) & bitmask) | twobit_repr('T');
  r = kmer_r >> 2 | (twobit_comp('T') << rc_left_shift);
  calc_connected_graph_size(f, r, count, keeper, threshold);

  // PREVIOUS.

  r = ((kmer_r << 2) & bitmask) | twobit_comp('A');
  f = kmer_f >> 2 | (twobit_repr('A') << rc_left_shift);
  calc_connected_graph_size(f, r, count, keeper, threshold);

  r = ((kmer_r << 2) & bitmask) | twobit_comp('C');
  f = kmer_f >> 2 | (twobit_repr('C') << rc_left_shift);
  calc_connected_graph_size(f, r, count, keeper, threshold);

  r = ((kmer_r << 2) & bitmask) | twobit_comp('G');
  f = kmer_f >> 2 | (twobit_repr('G') << rc_left_shift);
  calc_connected_graph_size(f, r, count, keeper, threshold);

  r = ((kmer_r << 2) & bitmask) | twobit_comp('T');
  f = kmer_f >> 2 | (twobit_repr('T') << rc_left_shift);
  calc_connected_graph_size(f, r, count, keeper, threshold);
}

void Hashbits::trim_graphs(const std::string infilename,
			    const std::string outfilename,
			    unsigned int min_size,
			    CallbackFn callback,
			    void * callback_data)
{
  IParser* parser = IParser::get_parser(infilename.c_str());
  unsigned int total_reads = 0;
  unsigned int reads_kept = 0;
  Read read;
  string seq;

  string line;
  ofstream outfile(outfilename.c_str());

  //
  // iterate through the FASTA file & consume the reads.
  //

  while(!parser->is_complete())  {
    read = parser->get_next_read();
    seq = read.seq;

    bool is_valid = check_read(seq);

    if (is_valid) {
      std::string first_kmer = seq.substr(0, _ksize);
      unsigned long long clustersize = 0;
      SeenSet keeper;
      calc_connected_graph_size(first_kmer.c_str(), clustersize, keeper,
				min_size);

      if (clustersize >= min_size) {
	outfile << ">" << read.name << endl;
	outfile << seq << endl;
	reads_kept++;
      }
    }
	       
    total_reads++;

    // run callback, if specified
    if (total_reads % CALLBACK_PERIOD == 0 && callback) {
      try {
	callback("trim_graphs", callback_data, total_reads, reads_kept);
      } catch (...) {
	delete parser;
	throw;
      }
    }
  }

  delete parser;
}

HashIntoType * Hashbits::graphsize_distribution(const unsigned int &max_size)
{
  HashIntoType * p = new HashIntoType[max_size];
  const unsigned char seen = 1 << 7;
  unsigned long long size;

  for (unsigned int i = 0; i < max_size; i++) {
    p[i] = 0;
  }

  for (HashIntoType i = 0; i < _tablesizes[0]; i++) {
    BoundedCounterType count = get_count(i);
    if (count && !(count & seen)) {
      std::string kmer = _revhash(i, _ksize);
      size = 0;

      SeenSet keeper;
      calc_connected_graph_size(kmer.c_str(), size, keeper, max_size);
      if (size) {
	if (size < max_size) {
	  p[size] += 1;
	}
      }
    }
  }

  return p;
}

void Hashbits::save_tagset(std::string outfilename)
{
  ofstream outfile(outfilename.c_str(), ios::binary);
  const unsigned int tagset_size = all_tags.size();

  HashIntoType * buf = new HashIntoType[tagset_size];

  outfile.write((const char *) &tagset_size, sizeof(tagset_size));
  outfile.write((const char *) &_tag_density, sizeof(_tag_density));

  unsigned int i = 0;
  for (SeenSet::iterator pi = all_tags.begin(); pi != all_tags.end();
	 pi++, i++) {
    buf[i] = *pi;
  }

  outfile.write((const char *) buf, sizeof(HashIntoType) * tagset_size);
  outfile.close();

  delete buf;
}

void Hashbits::load_tagset(std::string infilename)
{
  ifstream infile(infilename.c_str(), ios::binary);
  all_tags.clear();

  unsigned int tagset_size = 0;
  infile.read((char *) &tagset_size, sizeof(tagset_size));
  infile.read((char *) &_tag_density, sizeof(_tag_density));

  HashIntoType * buf = new HashIntoType[tagset_size];

  infile.read((char *) buf, sizeof(HashIntoType) * tagset_size);

  for (unsigned int i = 0; i < tagset_size; i++) {
    all_tags.insert(buf[i]);
  }
  
  delete buf;
}


void Hashbits::connectivity_distribution(const std::string infilename,
					 HashIntoType dist[9],
					 CallbackFn callback,
					 void * callback_data)
{
  const unsigned int rc_left_shift = _ksize*2 - 2;
  unsigned int total_reads = 0;
  for (unsigned int i = 0; i < 9; i++) {
    dist[i] = 0;
  }

  IParser* parser = IParser::get_parser(infilename);
  Read read;
  string seq;
  bool is_valid;

  HashIntoType kmer_f, kmer_r;

  while(!parser->is_complete()) {
    // increment read number
    read = parser->get_next_read();
    total_reads++;

    seq = read.seq;

    is_valid = check_read(seq);
    if (is_valid) {
      const char * kmer_s = seq.c_str();

      for (unsigned int i = 0; i < seq.length() - _ksize + 1; i++) {
	unsigned int neighbors = 0;
	_hash(kmer_s + i, _ksize, kmer_f, kmer_r);

	HashIntoType f, r;

	// NEXT.
	f = ((kmer_f << 2) & bitmask) | twobit_repr('A');
	r = kmer_r >> 2 | (twobit_comp('A') << rc_left_shift);
	if (get_count(uniqify_rc(f, r))) { neighbors++; }
	  
	f = ((kmer_f << 2) & bitmask) | twobit_repr('C');
	r = kmer_r >> 2 | (twobit_comp('C') << rc_left_shift);
	if (get_count(uniqify_rc(f, r))) { neighbors++; }

	f = ((kmer_f << 2) & bitmask) | twobit_repr('G');
	r = kmer_r >> 2 | (twobit_comp('G') << rc_left_shift);
	if (get_count(uniqify_rc(f, r))) { neighbors++; }

	f = ((kmer_f << 2) & bitmask) | twobit_repr('T');
	r = kmer_r >> 2 | (twobit_comp('T') << rc_left_shift);
	if (get_count(uniqify_rc(f, r))) { neighbors++; }

	// PREVIOUS.
	r = ((kmer_r << 2) & bitmask) | twobit_comp('A');
	f = kmer_f >> 2 | (twobit_repr('A') << rc_left_shift);
	if (get_count(uniqify_rc(f, r))) { neighbors++; }

	r = ((kmer_r << 2) & bitmask) | twobit_comp('C');
	f = kmer_f >> 2 | (twobit_repr('C') << rc_left_shift);
	if (get_count(uniqify_rc(f, r))) { neighbors++; }
    
	r = ((kmer_r << 2) & bitmask) | twobit_comp('G');
	f = kmer_f >> 2 | (twobit_repr('G') << rc_left_shift);
	if (get_count(uniqify_rc(f, r))) { neighbors++; }

	r = ((kmer_r << 2) & bitmask) | twobit_comp('T');
	f = kmer_f >> 2 | (twobit_repr('T') << rc_left_shift);
	if (get_count(uniqify_rc(f, r))) { neighbors++; }

	dist[neighbors]++;
      }

      // run callback, if specified
      if (total_reads % CALLBACK_PERIOD == 0 && callback) {
	try {
	  callback("connectivity_dist", callback_data, total_reads, 0);
	} catch (...) {
	  delete parser;
	  throw;
	}
      }
    }
  }

  delete parser;
}

//
// consume_fasta: consume a FASTA file of reads
//

void Hashbits::consume_fasta_and_tag(const std::string &filename,
				      unsigned int &total_reads,
				      unsigned long long &n_consumed,
				      CallbackFn callback,
				      void * callback_data)
{
  total_reads = 0;
  n_consumed = 0;

  IParser* parser = IParser::get_parser(filename.c_str());
  Read read;

  string seq = "";

  //
  // iterate through the FASTA file & consume the reads.
  //

  while(!parser->is_complete())  {
    read = parser->get_next_read();
    seq = read.seq;

    // yep! process.
    unsigned int this_n_consumed = 0;
    bool is_valid;

    this_n_consumed = check_and_process_read(seq, is_valid);
    n_consumed += this_n_consumed;
    if (is_valid) {
      const char * first_kmer = seq.c_str();

      unsigned char since = _tag_density;
      for (unsigned int i = 0; i < seq.length() - _ksize + 1; i++) {
	HashIntoType kmer = _hash(first_kmer + i, _ksize);
	if (all_tags.find(kmer) != all_tags.end()) {
	  since = 0;
	} else {
	  since++;
	}

	if (since >= _tag_density) {
	  all_tags.insert(kmer);
	  since = 0;
	}
      }
    }
	       
    // reset the sequence info, increment read number
    total_reads++;

    // run callback, if specified
    if (total_reads % CALLBACK_PERIOD == 0 && callback) {
      try {
        callback("consume_fasta_and_tag", callback_data, total_reads,
		 n_consumed);
      } catch (...) {
	delete parser;
        throw;
      }
    }
  }
  delete parser;
}

//
// thread_fasta: thread tags using a FASTA file of reads
//

void Hashbits::thread_fasta(const std::string &filename,
			    unsigned int &total_reads,
			    unsigned long long &n_consumed,
			    CallbackFn callback,
			    void * callback_data)
{
  total_reads = 0;
  n_consumed = 0;

  IParser* parser = IParser::get_parser(filename.c_str());
  Read read;

  string seq = "";

  //
  // iterate through the FASTA file
  //

  while(!parser->is_complete())  {
    SeenSet tags_to_join;

    read = parser->get_next_read();
    seq = read.seq;

    bool is_valid;

    is_valid = check_read(seq);

    if (is_valid) {
      const char * first_kmer = seq.c_str();

      for (unsigned int i = 0; i < seq.length() - _ksize + 1; i++) {
	HashIntoType kmer = _hash(first_kmer + i, _ksize);
	if (all_tags.find(kmer) != all_tags.end()) {
	  tags_to_join.insert(kmer);
	}
      }
    }

    HashIntoType kmer = *(tags_to_join.begin());
    partition->assign_partition_id(kmer, tags_to_join, false);
	       
    // reset the sequence info, increment read number
    total_reads++;

    // run callback, if specified
    if (total_reads % CALLBACK_PERIOD == 0 && callback) {
      try {
        callback("thread_fasta", callback_data, total_reads,
		 n_consumed);
      } catch (...) {
	delete parser;
        throw;
      }
    }
  }
  delete parser;
}

//
// divide_tags_into_subsets - take all of the tags in 'all_tags', and
//   divide them into subsets (based on starting tag) of <= given size.
//

void Hashbits::divide_tags_into_subsets(unsigned int subset_size,
					 SeenSet& divvy)
{
  unsigned int i = 0;

  for (SeenSet::const_iterator si = all_tags.begin(); si != all_tags.end();
       si++) {
    if (i % subset_size == 0) {
      divvy.insert(*si);
      i = 0;
    }
    i++;
  }
}

//
// tags_to_map - convert the 'all_tags' set into a TagCountMap, connecting
//    each tag to a number (defaulting to zero).
//

void Hashbits::tags_to_map(TagCountMap& tag_map)
{
  for (SeenSet::const_iterator si = all_tags.begin(); si != all_tags.end();
       si++) {
    tag_map[*si] = 0;
  }
  cout << "TM size: " << tag_map.size() << "\n";
}

//
// discard_tags - remove tags from a TagCountMap if they have fewer than
//   threshold count tags in their partition.  Used to eliminate tags belonging
//   to small partitions.
//

void Hashbits::discard_tags(TagCountMap& tag_map, unsigned int threshold)
{
  SeenSet delete_me;

  for (TagCountMap::const_iterator ti = tag_map.begin(); ti != tag_map.end();
       ti++) {
    if (ti->second < threshold) {
      delete_me.insert(ti->first);
    }
  }

  for (SeenSet::const_iterator si = delete_me.begin(); si != delete_me.end();
       si++) {
    tag_map.erase(*si);
  }
}

//
// consume_partitioned_fasta: consume a FASTA file of reads
//

void Hashbits::consume_partitioned_fasta(const std::string &filename,
					  unsigned int &total_reads,
					  unsigned long long &n_consumed,
					  CallbackFn callback,
					  void * callback_data)
{
  total_reads = 0;
  n_consumed = 0;

  IParser* parser = IParser::get_parser(filename.c_str());
  Read read;

  string seq = "";

  // reset the master subset partition
  delete partition;
  partition = new SubsetPartition(this);

  //
  // iterate through the FASTA file & consume the reads.
  //

  while(!parser->is_complete())  {
    read = parser->get_next_read();
    seq = read.seq;

    // yep! process.
    unsigned int this_n_consumed = 0;
    bool is_valid;

    this_n_consumed = check_and_process_read(seq, is_valid);
    n_consumed += this_n_consumed;
    if (is_valid) {
      // First, figure out what the partition is (if non-zero), and save that.
      PartitionID p = 0;

      const char * s = read.name.c_str() + read.name.length() - 1;
      assert(*(s + 1) == (unsigned int) NULL);

      while(*s != '\t' && s >= read.name.c_str()) {
	s--;
      }

      if (*s == '\t') {
	p = (PartitionID) atoi(s + 1);
      }

      // Next, compute the tags & set the partition, if nonzero
      const char * first_kmer = seq.c_str();
      for (unsigned int i = 0; i < seq.length() - _ksize + 1;
	   i += _tag_density) {
	HashIntoType kmer = _hash(first_kmer + i, _ksize);
	all_tags.insert(kmer);
	if (p > 0) {
	  partition->set_partition_id(kmer, p);
	}
      }
    }
	       
    // reset the sequence info, increment read number
    total_reads++;

    // run callback, if specified
    if (total_reads % CALLBACK_PERIOD == 0 && callback) {
      try {
        callback("consume_partitioned_fasta", callback_data, total_reads,
		 n_consumed);
      } catch (...) {
	delete parser;
        throw;
      }
    }
  }

  delete parser;
}