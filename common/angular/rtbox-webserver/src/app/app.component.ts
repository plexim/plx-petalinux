import { Component, OnInit, Inject, ViewChild } from '@angular/core';
import { Observable, BehaviorSubject } from 'rxjs';
import { TabsetComponent, TabDirective } from 'ngx-bootstrap/tabs';
import { switchMap } from 'rxjs/operators';

import { RefreshControlService } from './shared/refresh-control.service';
import { CONNECTION_STATUS } from './shared/connection-status';
import { RTBOX_TYPE } from './shared/rtbox-type';
import { RtboxXmlrpcService } from './shared/rtbox-xmlrpc.service';
import { SerialInfo, SimulationInfo } from './shared/interfaces';
import { FrontPanelComponent } from './front-panel/front-panel.component';

@Component({
  selector: 'rtb-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss']
})
export class AppComponent implements OnInit {
  readonly title = 'RTBox Webserver';
  private logPosition = 0;
  private modelTimeStamp = 0;
  hostname$: Observable<string>;
  serverStatusMsg: string;
  simInfo: SimulationInfo = undefined;
  statusText = '';
  statusError = false;
  statusRunning = false;
  refreshDiagnostics = false;
  @ViewChild(FrontPanelComponent) private frontPanelTab: FrontPanelComponent;
  @ViewChild('tabset') tabset: TabsetComponent;


  constructor(private rs: RtboxXmlrpcService,
              private refresh: RefreshControlService,
              @Inject(RTBOX_TYPE) public rtboxType: number,
              @Inject(CONNECTION_STATUS) status: BehaviorSubject<string>) {
    status.subscribe(message => this.serverStatusMsg = message);
  }

  ngOnInit(): void
  {
    this.hostname$ = this.rs.hostname();

    this.rs.boxType().subscribe(type => {
      if (type !== this.rtboxType) {
        alert(`Error: Page is configured for an RTBox-${this.rtboxType}, but the connected device is an RTBox-${type}`);
      }
    });
  }

  updateSimulationInfo(info: SimulationInfo): void
  {
    this.simInfo = info;
    this.updateStatus(info.status);
  }

  selectFrontPanel(data: TabDirective): void
  {
    this.frontPanelTab.refresh();
  }

  openTabFromStatus(): void
  {
    if (this.statusError)
    {
      this.tabset.tabs[3].active = true;
    }
    else if (this.statusRunning)
    {
      this.tabset.tabs[0].active = true;
    }
  }

  private updateStatus(newStatus: string)
  {
    this.statusError = false;
    this.statusRunning = false;

    if (newStatus === 'stopped')
    {
      this.statusText = 'No simulation running';
    }
    else if (newStatus === 'error')
    {
      this.statusText = 'Simulation error';
      this.statusError = true;
    }
    else if (newStatus === 'running')
    {
      this.statusText = 'Running';
      this.statusRunning = true;
    }
  }
}
