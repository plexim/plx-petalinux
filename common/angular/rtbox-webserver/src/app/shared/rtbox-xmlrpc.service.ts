import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';
import { map } from 'rxjs/operators';

import { XmlrpcService } from './xmlrpc.service';
import { SerialInfo, SimulationInfo, Status, PerfCounter } from './interfaces';
import { ResponseConversion } from './response-conversion';

@Injectable({
  providedIn: 'root'
})
export class RtboxXmlrpcService {
  private static extractRtboxType(info: SerialInfo): number
  {
    const from = info.rtboxType;

    if (from === 'PLECS RT Box CE') {
      return 0;
    } else {
      return parseInt(from.slice(from.length - 1), 10);
    }
  }

  constructor(private xmlrpc: XmlrpcService) {}

  boxType(): Observable<number> {
    return this.serialInfo().pipe(
      map(info => RtboxXmlrpcService.extractRtboxType(info)));
  }

  serialInfo(): Observable<SerialInfo> {
    return this.xmlrpc.call<SerialInfo>('rtbox.serials');
  }

  hostname(): Observable<string> {
    return this.xmlrpc.call<string>('rtbox.hostname');
  }

  querySimulation(): Observable<SimulationInfo> {
    return this.xmlrpc.call<SimulationInfo>('rtbox.querySimulation').pipe(
      map(info => ResponseConversion.adjustSampleTime<SimulationInfo>(info))
    );
  }

  status(modelTimeStamp: number, logPosition: number): Observable<Status> {
    return this.xmlrpc.call<Status>('rtbox.status', [modelTimeStamp, logPosition]);
  }

  queryCounter(): Observable<PerfCounter> {
    return this.xmlrpc.call<any>('rtbox.queryCounter').pipe(
      map(ResponseConversion.perfCounter),
    );
  }

  resetCounter(): Observable<{}> {
    return this.xmlrpc.call<{}>('rtbox.resetCounter');
  }

  reboot(): Observable<{}> {
    return this.xmlrpc.call<{}>('rtbox.reboot', [ 'reboot' ]);
  }
}
