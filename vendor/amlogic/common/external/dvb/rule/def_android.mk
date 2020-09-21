
JAVAC:=javac
JAR:=jar
DX:=$(ANDROID_HOSTDIR)/bin/dx
AIDL:=$(ANDROID_HOSTDIR)/bin/aidl

CFLAGS+=-I./

#Open this to get verbose javac output
#JAVACFLAGS+=-verbose

JAVACFLAGS+=-classpath $(JAVA_CLASSPATH)

