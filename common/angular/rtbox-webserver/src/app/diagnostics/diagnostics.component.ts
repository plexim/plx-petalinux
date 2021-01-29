import { Component, OnInit, Inject, Input } from '@angular/core';
import { switchMap, filter } from 'rxjs/operators';

import { RtboxXmlrpcService } from '../shared/rtbox-xmlrpc.service';
import { RefreshControlService } from '../shared/refresh-control.service';
import { Status } from '../shared/interfaces';
import { RTBOX_TYPE } from '../shared/rtbox-type';

class Row {
  public power: number;

  constructor(public voltage: number, public current: number)
  {
    this.power = this.voltage * this.current;
  }
}

@Component({
  selector: 'rtb-diagnostics',
  templateUrl: './diagnostics.component.html',
  styleUrls: ['./diagnostics.component.scss']
})
export class DiagnosticsComponent implements OnInit {
  @Input() refresh = false; // Send requests only when needed (controlled from the parent component)
  status: Status = {
    temperature: 0,
    fanSpeed: 0,
    logPosition: 0,
    applicationLog: '',
    clearLog: true,
    modelTimeStamp: 0,
    voltages: [],
    currents: []
  };
  temperatureMinMax = [0, 0];
  fanSpeedMinMax = [0, 0];
  readonly voltageRows = [3.3, 5.0, 12.5];
  powerData: Array<Row> = undefined;
  totalPower = 0;

  private static updateMinMax(newValue: number, minMax: number[]): void
  {
    // In this context, the max. value is not really tracked (it's always identical to the current value, which results
    // in no visual traces of the maximum). If desired, set the first element of minMax to the max of its current value
    // and the newValue.
    minMax[0] = newValue;
    minMax[1] = newValue;
  }

  constructor(private rs: RtboxXmlrpcService,
              private refreshService: RefreshControlService,
              @Inject(RTBOX_TYPE) public rtboxType: number) {}

  ngOnInit(): void {
    const timerToStatus = t => this.rs.status(this.status.modelTimeStamp, this.status.logPosition);
    const status$ = this.refreshService.timer().pipe(
      filter(() => this.refresh),
      switchMap(timerToStatus));

    status$.subscribe(newStatus => this.updateStatus(newStatus));
  }

  private updateStatus(newStat: Status): void {
    this.status.temperature = newStat.temperature;
    this.status.fanSpeed = newStat.fanSpeed;

    DiagnosticsComponent.updateMinMax(newStat.temperature, this.temperatureMinMax);
    DiagnosticsComponent.updateMinMax(newStat.fanSpeed, this.fanSpeedMinMax);

    if (newStat.clearLog) {
      this.status.modelTimeStamp = newStat.modelTimeStamp;
      this.status.applicationLog = newStat.applicationLog;
      this.status.logPosition = newStat.logPosition;
    } else if (newStat.logPosition < this.status.logPosition) {
      this.status.applicationLog = '';
    }  else {
      this.status.logPosition = newStat.logPosition;
      this.status.applicationLog += newStat.applicationLog;
    }

    if (this.rtboxType > 1) {
      this.updatePowerConsumption(newStat);
    }
  }

  private updatePowerConsumption(newStat: Status): void {
    this.powerData = new Array<Row>();

    for (let i = 0; i < newStat.voltages.length; ++i) {
      this.powerData.push(new Row(newStat.voltages[i], newStat.currents[i]));
    }

    this.totalPower = this.powerData.reduce((soFar, row) => soFar + row.power, 0);
  }
}
