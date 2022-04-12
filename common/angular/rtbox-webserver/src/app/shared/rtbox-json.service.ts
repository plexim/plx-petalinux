import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';
import { map } from 'rxjs/operators';

import { SerialInfo, SimulationInfo, Status, PerfCounter } from './interfaces';
import { RtboxCommunicationService } from './rtbox-communication.service';
import { ResponseConversion } from './response-conversion';
import { JsonRpcService } from './json.service';

@Injectable({
  providedIn: 'root'
})

export class RtboxJsonService implements RtboxCommunicationService {
  private static extractRtboxType(info: SerialInfo): number
  {
    const from = info.rtboxType;

    if (from === 'PLECS RT Box CE') {
      return 0;
    } else {
      return parseInt(from.slice(from.length - 1), 10);
    }
  }

  constructor(private jsonRpc: JsonRpcService) {}

  boxType(): Observable<number> {
    return this.serialInfo().pipe(
      map(info => RtboxJsonService.extractRtboxType(info)));
  }

  serialInfo(): Observable<SerialInfo> {
    return this.jsonRpc.call<SerialInfo>('rtbox.serials');
  }

  hostname(): Observable<string> {
    return this.jsonRpc.call<string>('rtbox.hostname');
  }

  querySimulation(): Observable<SimulationInfo> {
    return this.jsonRpc.call<SimulationInfo>('rtbox.querySimulation').pipe(
      map(info => ResponseConversion.adjustSampleTime<SimulationInfo>(info))
    );
  }

  status(modelTimeStamp: number, logPosition: number): Observable<Status> {
    return this.jsonRpc.call<Status>('rtbox.status', [modelTimeStamp, logPosition]);
  }

  queryCounter(): Observable<PerfCounter> {
    return this.jsonRpc.call<any>('rtbox.queryCounter').pipe(
      map(ResponseConversion.perfCounter),
    );
  }

  resetCounter(): Observable<{}> {
    return this.jsonRpc.call<{}>('rtbox.resetCounter');
  }

  reboot(): Observable<{}> {
    return this.jsonRpc.call<{}>('rtbox.reboot', [ 'reboot' ]);
  }
}
