GIT_SSL_CAINFO = ${PETALINUX}/components/yocto/source/aarch64/buildtools/sysroots/x86_64-petalinux-linux/etc/ssl/certs/ca-certificates.crt

all:
	cd rtbox-webserver && /usr/bin/scl enable rh-nodejs10 \
	 "export GIT_SSL_CAINFO=${GIT_SSL_CAINFO}; echo n | /opt/rh/rh-nodejs10/root/usr/bin/npm install"
	cp rtbox-webserver/src/app/app.module.ts.rtbox1 rtbox-webserver/src/app/app.module.ts
	cd rtbox-webserver && /usr/bin/scl enable rh-nodejs10 "/opt/rh/rh-nodejs10/root/usr/bin/ng build --prod --output-path rtbox1 --output-hashing=bundles"
	cp rtbox-webserver/src/app/app.module.ts.rtboxce rtbox-webserver/src/app/app.module.ts
	cd rtbox-webserver && /usr/bin/scl enable rh-nodejs10 "/opt/rh/rh-nodejs10/root/usr/bin/ng build --prod --output-path rtboxce --output-hashing=bundles"
	mv rtbox-webserver/rtbox1/index.html rtbox-webserver/rtbox1/index.html.rtbox1
	mv rtbox-webserver/rtboxce/index.html rtbox-webserver/rtboxce/index.html.rtboxce
	cp -r rtbox-webserver/rtboxce/* rtbox-webserver/rtbox1

clean:
	-rm -r rtbox-webserver/rtbox1
	-rm -r rtbox-webserver/rtboxce

