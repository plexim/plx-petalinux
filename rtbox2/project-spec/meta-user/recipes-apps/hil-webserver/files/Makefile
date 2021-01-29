all:
	cd rtbox-webserver && /usr/bin/scl enable rh-nodejs10 "echo n | /opt/rh/rh-nodejs10/root/usr/bin/npm install"
	cp rtbox-webserver/src/app/app.module.ts.rtbox3 rtbox-webserver/src/app/app.module.ts
	cd rtbox-webserver && /usr/bin/scl enable rh-nodejs10 "/opt/rh/rh-nodejs10/root/usr/bin/ng build --prod --output-path rtbox3 --output-hashing=bundles"
	cp rtbox-webserver/src/app/app.module.ts.rtbox2 rtbox-webserver/src/app/app.module.ts
	cd rtbox-webserver && /usr/bin/scl enable rh-nodejs10 "/opt/rh/rh-nodejs10/root/usr/bin/ng build --prod --output-path rtbox2 --output-hashing=bundles"
	mv rtbox-webserver/rtbox3/index.html rtbox-webserver/rtbox3/index.html.rtbox3	
	mv rtbox-webserver/rtbox2/index.html rtbox-webserver/rtbox2/index.html.rtbox2	
	cp -r rtbox-webserver/rtbox3/* rtbox-webserver/rtbox2

clean:
	-rm -r rtbox-webserver/rtbox2
	-rm -r rtbox-webserver/rtbox3

