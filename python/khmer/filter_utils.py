def filter_fasta_file_any(ht, filename, total_reads, outname, threshold):
    minmax = ht.fasta_file_to_minmax(filename, total_reads)
    readmask = ht.filter_fasta_file_any(minmax, threshold)

    n_kept = readmask.filter_fasta_file(filename, outname)

    return total_reads, n_kept

def filter_fasta_file_all(ht, filename, total_reads, outname, threshold):
    minmax = ht.fasta_file_to_minmax(filename, total_reads)
    readmask = ht.filter_fasta_file_all(minmax, threshold)

    n_kept = readmask.filter_fasta_file(filename, outname)

    return total_reads, n_kept

def filter_fasta_file_limit_n(ht, filename, total_reads, outname, threshold, n):
    minmax = ht.fasta_file_to_minmax(filename, total_reads)
    readmask = ht.filter_fasta_file_limit_n(filename, minmax, threshold, n)

    n_kept = readmask.filter_fasta_file(filename, outname)

    return total_reads, n_kept
