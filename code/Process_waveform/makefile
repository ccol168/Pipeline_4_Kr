LIBS:=`root-config --libs`
INCS:=`root-config --cflags`

do: 
	make Process_waveform


Process_waveform: Process_waveform.o JUNO_PMTs.o
	g++ Process_waveform.o JUNO_PMTs.o -o Process_waveform.x -lSpectrum ${INCS} ${LIBS}

JUNO_PMTs.o: JUNO_PMTs.cxx JUNO_PMTs.h 
	g++ -c JUNO_PMTs.cxx -o JUNO_PMTs.o ${INCS} ${LIBS}
Process_waveform.o: Process_waveform.cxx 
	g++ -c Process_waveform.cxx -o Process_waveform.o -lSpectrum ${INCS} ${LIBS}
