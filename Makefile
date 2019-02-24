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
	install -m 0755 ${prog} ${prefix}/bin
	install -m 0755 clockc.sh ${prefix}/bin
	mkdir -p /etc/init.d
	install -m 0755 clockd-server.openrc /etc/init.d
	install -m 0755 clockd-client.openrc /etc/init.d
	mkdir -p /usr/lib/systemd/system
	install -m 0644 clockd-server.service /usr/lib/systemd/system
	install -m 0644 clockd-client.service /usr/lib/systemd/system
	install -m 0644 clockd-client.timer /usr/lib/systemd/system
	install -m 0644 clockd.txt /etc
