man_MANS = lmfit.7 lmmin.3 lmcurve.3
noinst_DATA = lmfit.html lmmin.html lmcurve.html

%.3 : %.pod
	pod2man -s 3 -c "lmfit manual" -n "$*"      $< > $@

%.7 : %.pod
	pod2man -s 7 -c "lmfit manual" -n "$*"      $< > $@

%.html : %.pod
	pod2html --title="lmfit: a self-contained C library for Levenberg-Marquardt least-squares minimization and curve fitting" --noindex $< > $@
