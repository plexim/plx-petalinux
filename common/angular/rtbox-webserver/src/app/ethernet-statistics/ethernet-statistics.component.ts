import { Component, OnInit } from '@angular/core';
import { Observable, Subscription } from 'rxjs';

import { RtboxCgiService } from '../shared/rtbox-cgi.service';
import { RefreshControlService } from '../shared/refresh-control.service';
import { NetState } from '../shared/interfaces';

@Component({
  selector: 'rtb-ethernet-statistics',
  templateUrl: './ethernet-statistics.component.html',
  styleUrls: ['./ethernet-statistics.component.scss']
})
export class EthernetStatisticsComponent implements OnInit {
  data$: Observable<NetState>;
  ipAddress = '';
  data: NetState = null;
  private timerSubscription: Subscription;

  constructor(private cgi: RtboxCgiService, private refresh: RefreshControlService) {}

  ngOnInit(): void
  {
    this.timerSubscription = this.refresh.timer().subscribe(t => this.updateNetStatistics());
  }

  private updateNetStatistics(): void
  {
    // We resolve the observables right here instead of piping them through 'async' in the template, as this makes the
    // modal dialog show an empty table for some subsecond interval.
    this.cgi.netstate().subscribe( netStateResponse => this.data = netStateResponse);
    this.cgi.ipstate().subscribe(addr => this.ipAddress = addr);
  }

  ngOnDestroy(): void
  {
    this.timerSubscription.unsubscribe();
  }
}

