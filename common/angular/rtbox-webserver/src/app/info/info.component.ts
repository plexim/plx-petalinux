import { Component, OnInit } from '@angular/core';
import { RtboxCommunicationService } from '../shared/rtbox-communication.service';
import { Observable } from 'rxjs';

import { SerialInfo, SimulationInfo } from '../shared/interfaces';

@Component({
  selector: 'rtb-info',
  templateUrl: './info.component.html',
  styleUrls: ['./info.component.scss']
})
export class InfoComponent implements OnInit {
  readonly ipAddrWording = { '=1': 'IP Address', other: 'IP Addresses' };
  hostname$: Observable<string>;
  serials$: Observable<SerialInfo>;
  info$: Observable<SimulationInfo>;

  constructor(private rs: RtboxCommunicationService) {}

  ngOnInit(): void {
    this.hostname$ = this.rs.hostname();
    this.serials$ = this.rs.serialInfo();
    this.info$ = this.rs.querySimulation();
  }
}
