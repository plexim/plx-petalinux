import { Injectable, Inject } from '@angular/core';
import { HttpClient, HttpHeaders } from '@angular/common/http';

import { Observable } from 'rxjs';
import { bindCallback } from 'rxjs';
import { map, mergeMap, tap } from 'rxjs/operators';

import * as serialize from 'xmlrpc/lib/serializer';
import * as Deserializer from 'xmlrpc/lib/deserializer';
import { Readable } from 'readable-stream';

@Injectable({
  providedIn: 'root'
})
export class XmlrpcService {
  private deserializer: any;

  constructor(private http: HttpClient, @Inject(Window) private window: Window) {}

  call<T>(method: string, command: Array<any> = []): Observable<T>
  {
    const call = this.serializeCall(method, command);

    return this.postXmlAndDeserialize(call);
  }

  close(uid: string): Observable<any>
  {
    const call = this.serializeCall('close', [uid]);

    return this.postXmlAndDeserialize(call);
  }

  private serializeCall(methodName: string, command: any): string
  {
    const call = serialize.serializeMethodCall(methodName, command);

    return call;
  }

  private postXmlAndDeserialize(body: string): Observable<any>
  {
    const contentTypeHeader = new HttpHeaders().set('Content-Type', 'application/xml');
    /* When working with a local Angular development server, this must simply be 'rpc2', accompanied by a suitable
     * configuration in proxy.conf.json. This is probably not super robust, as accessing the page via 127.0.0.1:4200
     * will already break the mechanism - but for the time being, it's enough. */
    const url = this.window.location.hostname === 'localhost' ? '/rpc2' : `http://${this.window.location.hostname}:9998/rpc2`;
    const response = this.http.post(url, body, {headers: contentTypeHeader, responseType: 'text'});
    const deserializeMap = mergeMap(this.deserialize.bind(this));

    return response.pipe(deserializeMap);
  }

  private deserialize(body: string): Observable<any>
  {
    /* The external deserialization functionality requires a stream object to be passed in (and the
     * underlying SAX parser needs that, too). In Node, creating a stream from a string (i.e., the
     * response body) works with Readable.fromString, but for some reason, this isn't available in
     * the browser. Don't ask my why the following workaround satisfies these needs (copy-pasted
     * from somewhere), but it does. */
    const input = new Readable({
      read() {
        this.push(body);
        this.push(null);
      }
    });

    const ds = new Deserializer();
    /* The deserialization backend does its work asynchronously, we must hence process the result as
     * a callback. This logic is transformed into an Observable. */
    const backendDeserialize = (f) => { ds.deserializeMethodResponse(input, f); };
    const callbackToObservable = bindCallback(backendDeserialize);
    const throwOrResult = ([err, result]) => { if (err) { throw err; } return result; };

    return callbackToObservable().pipe(map(throwOrResult));
  }
}
