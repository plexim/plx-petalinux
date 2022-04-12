import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';

import { SerialInfo, SimulationInfo, Status, PerfCounter } from './interfaces';

@Injectable({
  providedIn: 'root'
})

export abstract class RtboxCommunicationService {

  abstract boxType(): Observable<number>;

  abstract serialInfo(): Observable<SerialInfo>;

  abstract hostname(): Observable<string>;

  abstract querySimulation(): Observable<SimulationInfo>;

  abstract status(modelTimeStamp: number, logPosition: number): Observable<Status>;

  abstract queryCounter(): Observable<PerfCounter>;

  abstract resetCounter(): Observable<{}>;

  abstract reboot(): Observable<{}>;
}
