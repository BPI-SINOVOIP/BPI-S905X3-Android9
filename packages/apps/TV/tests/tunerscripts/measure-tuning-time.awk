# Awk script to measure tuning time statistics from logcat dump

BEGIN {
  n = 0;
  sum = 0;
}

# Collect tuning time with "Video available in <time> ms" message
/Video available in/ {
  n++;
  tune_time = $11;
  sum += tune_time;
  if (n == 1) {
    min = tune_time;
    max = tune_time;
  } else {
    if (tune_time < min) {
      min = tune_time
    }
    if (tune_time > max) {
      max = tune_time
    }
  }
}

END {
  average = sum / n;
  print "Average tune time", average, "ms";
  print "Minimum tune time", min, "ms";
  print "Maximum tune time", max, "ms";
}

