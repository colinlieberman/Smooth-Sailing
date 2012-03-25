CC          = gcc
CXX         = g++
INCPATH     = -I../xsdk/CHeaders/XPLM -I../xsdk/CHeaders/Wrappers -I../xsdk/CHeaders/Widgets -I.
DEFINES     = -DXPLM200 -DAPL=1 -DIBM=0 -DLIN=0
CXXFLAGS    = -pipe -Os -arch i386 -Wall -W -fPIC $(DEFINES)
OBJECT      = smoothsailing.mac.xpl

smoothsailing: ss.cpp ss.h
	$(CXX) -c $(CXXFLAGS) -o $(OBJECT) ss.cpp $(INCPATH)

clean: 
	rm $(OBJECT)
