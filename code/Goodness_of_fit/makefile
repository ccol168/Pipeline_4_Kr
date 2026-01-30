LIBS:=`root-config --libs`
INCS:=`root-config --cflags`

do: 
	make Calculate_fit_goodness


Calculate_fit_goodness: Calculate_fit_goodness.o 
	g++ Calculate_fit_goodness.o -o Calculate_fit_goodness.x -lSpectrum ${INCS} ${LIBS}

Calculate_fit_goodness.o: Calculate_fit_goodness.cxx 
	g++ -c Calculate_fit_goodness.cxx -o Calculate_fit_goodness.o -lSpectrum ${INCS} ${LIBS}