import { Injectable } from '@angular/core';
import { Observable, of } from 'rxjs';

import { FrontPanelState, NetState } from './interfaces';

@Injectable()
export class RtboxCgiStubService {
  constructor() { }

  upload(payload: File, startAfterUpload: boolean): Observable<string>
  {
    return of('');
  }

  startExecution(startOnFirstTrigger: boolean): Observable<string>
  {
    return of('');
  }

  stopExecution(): Observable<string>
  {
    return of('');
  }

  netstate(): Observable<NetState>
  {
    const rand = max => Math.floor(Math.random() * Math.floor(max));

    const result: NetState = {
      speed: rand(1000),
      duplex: 'full',
      rx_errors: rand(50),
      tx_errors: rand(50),
      rxp: rand(50),
      txp: rand(50),
      collisions: rand(100)
    };

    return of(result);
  }

  ipstate(): Observable<string>
  {
    return of('192.168.0.123');
  }

  frontPanel(): Observable<FrontPanelState>
  {
    const voltages = [ 1, 2, 3, 4, 5, 6, 11, 12, 13];

    const result: FrontPanelState = {
      leds: {
        error: false,
        running: false,
        ready: true,
        power: true
      },
      voltage: {
        analogIn: voltages[Math.floor(Math.random() * voltages.length)],
          analogOut: voltages[Math.floor(Math.random() * voltages.length)],
          digitalIn: 13,
          digitalOut: voltages[Math.floor(Math.random() * voltages.length)]
      }
    };

    return of(result);
  }
}
