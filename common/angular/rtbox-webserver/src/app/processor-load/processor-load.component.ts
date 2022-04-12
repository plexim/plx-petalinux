import { Component, OnInit, Output, EventEmitter, Inject } from '@angular/core';
import { RtboxCommunicationService } from '../shared/rtbox-communication.service';
import { switchMap } from 'rxjs/operators';

import { RefreshControlService } from '../shared/refresh-control.service';
import { PerfCounter, SimulationInfo } from '../shared/interfaces';
import { RTBOX_TYPE } from '../shared/rtbox-type';

export class ExecTime {
  constructor(public maximum: number, public current: number) {}
}

@Component({
  selector: 'rtb-processor-load',
  templateUrl: './processor-load.component.html',
  styleUrls: ['./processor-load.component.scss']
})
export class ProcessorLoadComponent implements OnInit {
  /* Note: the initialization order here matters. First, constructor shorthand parameters are initialized (the
   * constructor arguments with private/public specifier). Then, class members in their order of declaration. And last,
   * the constructor body. As the following class member depend on each other and on the constructor parameter, don't
   * reorder them without a specific need or without care. */
  readonly nProc = this.rtboxType <= 1 ? 1 : 3;
  private sampleTime = 0;
  private savedModelTimeStamp = 0;
  private periodTicks = new Array<number>(this.nProc).fill(1);
  readonly historyLength = 21;
  loadValues = this.createArrayPerCore<number[]>(() => [0, 0]);
  clampedLoadValues = this.createArrayPerCore<number[]>(() => [0, 0]);
  loadValuesHistory = this.createArrayPerCore<ExecTime[]>(() => []);
  currentCycleTimes = this.createArrayPerCore<number>(() => 0);
  maxCycleTimes = this.createArrayPerCore<number>(() => 0);
  @Output() simulationInfoBroadcast = new EventEmitter<SimulationInfo>();

  private createArrayPerCore<T>(factory: (() => T)): Array<T>
  {
    const array = new Array<T>(this.nProc);

    /* The .fill .map combination is important here as we possibly want to initialize the array elements with
     * non-primitive data, i.e. objects/arrays. In this case, passing a literal or bound value to .fill (e.g. .fill([]))
     * will cause one single object being referred to by all array elements. Hence, we defer constructing the new data
     * to the factory function: */
    return array.fill(undefined).map(factory);
  }

  constructor(private rs: RtboxCommunicationService,
              private refresh: RefreshControlService,
              @Inject(RTBOX_TYPE) public rtboxType: number) {}

  ngOnInit(): void {
    this.refreshSimulationInfo();

    const counter$ = this.refresh.timer().pipe(
      switchMap(t => this.rs.queryCounter()));

    counter$.subscribe(newCounter => this.updateCounter(newCounter));
  }

  private refreshSimulationInfo(): void {
    this.rs.querySimulation().subscribe(info => this.updateSimulationInfo(info));
  }

  private updateSimulationInfo(info: SimulationInfo): void {
    if (info.periodTicks) {
      this.periodTicks = info.periodTicks.map(value => value ? value : 1);
    }
    else {
      this.periodTicks[0] = info.sampleTime ? info.sampleTime : 1;
    }

    this.savedModelTimeStamp = info.modelTimeStamp;
    this.sampleTime = info.sampleTime;

    this.simulationInfoBroadcast.emit(info);
  }

  private updateCounter(newCounter: PerfCounter): void {
    this.currentCycleTimes = new Array<number>();
    this.maxCycleTimes = new Array<number>();

    const factor = this.sampleTime/this.periodTicks[0] / 1000;
    for (let core = 0; core < this.nProc; ++core) {
      this.loadValues[core] = [
        newCounter.maxCycleTime[core] / this.periodTicks[core] * 100,
        newCounter.runningCycleTime[core] / this.periodTicks[core] * 100,
      ];

      this.clampedLoadValues[core] = this.loadValues[core].map(
        value => value < 0 ? 0 : (value > 100 ? 100 : value));

      this.currentCycleTimes.push(newCounter.runningCycleTime[core] * factor);
      this.maxCycleTimes.push(newCounter.maxCycleTime[core] * factor);
    }

    for (let core = 0; core < this.nProc; ++core) {
      const coreHistory = this.loadValuesHistory[core];
      const maximum = this.loadValues[core][0];
      const current = this.loadValues[core][1];

      coreHistory.push(new ExecTime(maximum, current));

      if (coreHistory.length > this.historyLength) {
        coreHistory.splice(0, coreHistory.length - this.historyLength);
      }
    }

    if (this.savedModelTimeStamp !== newCounter.modelTimeStamp) {
      this.refreshSimulationInfo();
    }
  }

  resetMaxima(): void {
    this.rs.resetCounter().subscribe(() => undefined, console.error);

    this.maxCycleTimes = this.currentCycleTimes;

    for (const values of [this.loadValues, this.clampedLoadValues]) {
      for (const maxCurrent of values) {
        maxCurrent[0] = maxCurrent[1];
      }
    }
  }
}
