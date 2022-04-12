import { Injectable, Inject } from '@angular/core';
import { HttpClient, HttpHeaders } from '@angular/common/http';

import { Observable } from 'rxjs';
import { bindCallback } from 'rxjs';
import { map, mergeMap, tap } from 'rxjs/operators';

class JsonRpc
{
  jsonrpc: string = '2.0';
  method: string;
  params: Array<any>;
  id: number;
}

@Injectable({
  providedIn: 'root'
})
export class JsonRpcService {

  private id = 0;

  constructor(private http: HttpClient, @Inject(Window) private window: Window) {}

  call<T>(method: string, command: Array<any> = []): Observable<T>
  {
    let jsonRpc = new JsonRpc();
    jsonRpc.id = this.id++;
    jsonRpc.method = method;
    jsonRpc.params = command;

    return this.postJson(jsonRpc);
  }

  close(uid: string): Observable<any>
  {
    let jsonRpc = new JsonRpc();
    jsonRpc.id = this.id++;
    jsonRpc.method = 'close';
    jsonRpc.params = new Array<string>();
    jsonRpc.params.push(uid);

    return this.postJson(jsonRpc);
  }

  private postJson(jsonRpc: JsonRpc): Observable<any>
  {
    let body = JSON.stringify(jsonRpc);
    const contentTypeHeader = new HttpHeaders().set('Content-Type', 'application/xml');
    /* When working with a local Angular development server, this must simply be 'rpc2', accompanied by a suitable
     * configuration in proxy.conf.json. This is probably not super robust, as accessing the page via 127.0.0.1:4200
     * will already break the mechanism - but for the time being, it's enough. */
    const url = this.window.location.hostname === 'localhost' ? '/rpc2' : `http://${this.window.location.hostname}:9998/rpc2`;
    const response = this.http.post(url, body, {headers: contentTypeHeader, responseType: 'json'});
    const readResponseMap = map(this.readResponse.bind(this));
    //return response.pipe(map(data => data["result"]));
    return response.pipe(readResponseMap);
  }

  private readResponse(response: JSON) : Observable<any>
  {
    if (response["result"] != null)
    {
      return response["result"];
    }
    else
    {
      throw(response["error"]);
    }
  }
}
