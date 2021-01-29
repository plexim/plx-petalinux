import { Injectable } from '@angular/core';
import { HttpClient, HttpParams } from '@angular/common/http';
import { Observable } from 'rxjs';
import { timeout, map, flatMap, tap } from 'rxjs/operators';

import { FrontPanelState, NetState } from './interfaces';
import { ResponseConversion } from './response-conversion';

@Injectable({
  providedIn: 'root'
})
export class RtboxCgiService {
  private readonly options = { responseType : 'text' as const }; // for empty responses
  private readonly urlPrefix = '/cgi-bin';

  constructor(private http: HttpClient) {}

  upload(payload: File, startAfterUpload: boolean, delayUntilScopeTrigger: boolean): Observable<string>
  {
    const formData = new FormData();

    formData.append('filename', payload, payload.name);

    const request$ = this.http.post(`${this.urlPrefix}/upload.cgi`,
                                    formData, this.options);

    const timeoutRequest$ = request$.pipe(timeout(10000));

    if (startAfterUpload) {
      return timeoutRequest$.pipe(flatMap(resp => this.startExecution(delayUntilScopeTrigger)));
    } else {
      return timeoutRequest$;
    }
  }

  startExecution(startOnFirstTrigger: boolean): Observable<string>
  {
    const params = new HttpParams({ fromObject: { startOnFirstTrigger: startOnFirstTrigger.toString() } });

    const request$ = this.http.post(`${this.urlPrefix}/start.cgi`,
                                    params, this.options);

    return request$.pipe(timeout(5000));
  }

  stopExecution(): Observable<string>
  {
    const request$ = this.http.get(`${this.urlPrefix}/stop.cgi`, this.options);

    return request$.pipe(timeout(3000));
  }

  netstate(): Observable<NetState>
  {
    return this.http.get<NetState>(`${this.urlPrefix}/netstate.cgi`);
  }

  ipstate(): Observable<string>
  {
    interface IpState {
      ip: string;
    }

    const result$ = this.http.get<IpState>(`${this.urlPrefix}/ipstate.cgi`);

    return result$.pipe(map(data => data.ip));
  }

  frontPanel(): Observable<FrontPanelState>
  {
    return this.http.get<FrontPanelState>(`${this.urlPrefix}/front-panel.cgi`).pipe(
      map(ResponseConversion.frontPanelState));
  }
}
