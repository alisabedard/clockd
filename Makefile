prefix=/usr/local
prog=clockd
${prog}: ${prog}.o
	${CXX} ${LDFLAGS} -o ${prog} ${prog}.o
clean:
	rm -f ${prog} ${prog}.o
install:
	install -m 0755 ${prog} ${prefix}/bin/
