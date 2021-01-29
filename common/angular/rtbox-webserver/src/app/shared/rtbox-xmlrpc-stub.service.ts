import { Injectable, Inject } from '@angular/core';
import { Observable, of } from 'rxjs';
import { map } from 'rxjs/operators';

import { RTBOX_TYPE } from '../shared/rtbox-type';

import { SerialInfo, SimulationInfo, Status, PerfCounter } from './interfaces';

@Injectable()
export class RtboxXmlrpcStubService {
  private readonly fixedFakeSampleTime = 2000;
  private currentLoad = [0.5 * Math.random(), 0.25 * Math.random(), 0.33 * Math.random()];
  private maxLoad = [0.0, 0.0, 0.0];

  constructor(@Inject(RTBOX_TYPE) private rtboxType: number) {}

  boxType(): Observable<number> {
    return of(this.rtboxType);
  }

  serialInfo(): Observable<SerialInfo> {
    const info: SerialInfo = {
      cpu: 2,
      mac: 'a1:b2:c3:d4:e5:f0',
      boardRevision: '1.2 Rev. 3',
      firmwareVersion: '4.5.6',
      firmwareBuild: 'r123456 01.01.2020',
      fpgaVersion: '7.8.9',
      ipAddresses: ['192.168.0.123', '192.168.0.124'],
      board: 123,
      rtboxType: 'PLECS RT Box 2'
    };

    return of(info);
  }

  hostname(): Observable<string> {
    return of('rtbox-stub');
  }

  querySimulation(): Observable<SimulationInfo> {
    const voltages = this.rtboxType > 1 ? [ 1, 2, 3, 4, 5, 6, 11, 12, 13] : [0];
    const info: SimulationInfo = {
      sampleTime: this.fixedFakeSampleTime,
      modelName: 'some-plecs-model',
      applicationVersion: '1.2.3',
      modelTimeStamp: this.timeStamp(),
      periodTicks: undefined,
      analogInVoltageRange: voltages[Math.floor(Math.random() * voltages.length)],
      analogOutVoltageRange: voltages[Math.floor(Math.random() * voltages.length)],
      digitalOutVoltage:  voltages[Math.floor(Math.random() * voltages.length)],
      status: undefined
    };

    if (this.rtboxType > 1) {
      info.periodTicks =  [2999, 2999, 2999];
    }
    info.status = 'running';

    return of(info);
  }

  status(modelTimeStamp: number, logPosition: number): Observable<Status> {
    const random = (this.timeStamp() % 60) / 60 * 2 * Math.PI;
    const randInt = max => Math.floor(Math.random() * Math.floor(max));

    const info: Status = {
      temperature: randInt(100),
      fanSpeed: randInt(6000),
      logPosition: 0,
      applicationLog: '',
      clearLog: false,
      modelTimeStamp: this.timeStamp(),
      voltages: undefined,
      currents: undefined
    };

    if (this.rtboxType > 1) {
      info.voltages = [42.0, 43.0, 44.0];
      info.currents = [1.234, 5.678, 9.876];
    }

    this.randomizeLogs(info);

    return of(info);
  }

  private timeStamp(): number
  {
    const now = new Date();

    return Math.round(now.getTime() / 1000);
  }

  private randomizeLogs(out: Status): void {
    const exampleText = [
      'Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do',
      ' eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim',
      ' ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut',
      ' aliquip ex ea commodo consequat. Duis aute irure dolor in',
      ' reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla',
      ' pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa',
      'qui officia deserunt mollit anim id est laborum.'
    ];
    const nMaxNewLogs = 10;
    const nNewLogs = Math.floor(Math.random() * nMaxNewLogs);
    out.clearLog = Math.random() < 0.2 ? true : false;

    if (nNewLogs === 0) {
      out.applicationLog = '';

      return;
    }

    for (let i = 0; i < nNewLogs; ++i) {
      const now = new Date();
      const text = exampleText[Math.floor(Math.random() * exampleText.length)];

      out.applicationLog += `[${now.toDateString()}] ${text}\n`;
    }
  }

  queryCounter(): Observable<PerfCounter> {
    const result: PerfCounter = {
      modelTimeStamp: this.timeStamp(),
      maxCycleTime: new Array<number>(),
      runningCycleTime: new Array<number>()
    };


    for (let i = 0; i < this.currentLoad.length; ++i) {
      const factor = 0.8 + 0.4 * Math.random();
      const summand = 0.2 * Math.random();
      const current = this.currentLoad[i];
      const next = Math.max(0, Math.min(factor * current + summand, 1));
      const nextMax = Math.max(this.maxLoad[i], next);

      result.maxCycleTime.push(nextMax * this.fixedFakeSampleTime);
      result.runningCycleTime.push(next * this.fixedFakeSampleTime);

      this.maxLoad[i] = nextMax;
    }

    return of(result);
  }

  resetCounter(): Observable<{}> {
    for (let i = 0; i < this.maxLoad.length; ++i) {
      this.maxLoad[i] = this.currentLoad[i];
    }

    return of();
  }

  reboot(): Observable<{}> {
    return of({});
  }
}
