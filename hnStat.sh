hnStatTop() { cut -d'	' -f2 $1 | sort | uniq -c | sort -n -k 1 -r | head -n $2; }
hnStatDistinct() { cut -d'	' -f2 $1 | sort -u | wc -l; }
