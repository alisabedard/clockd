prefix=/usr/local
prog=clockd
#CFLAGS+="--std=c11 -D_POSIX_C_SOURCE=200809L"
CFLAGS+=--std=c11
CFLAGS+=-D_POSIX_C_SOURCE=200809L
CFLAGS+=-D_DEFAULT_SOURCE
${prog}: ${prog}.o
	${CC} ${LDFLAGS} -o ${prog} ${prog}.o
clean:
	rm -f ${prog} ${prog}.o
install:
	install -m 0755 ${prog} ${prefix}/bin/
