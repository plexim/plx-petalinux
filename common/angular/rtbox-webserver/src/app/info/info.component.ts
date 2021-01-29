import { Component, OnInit } from '@angular/core';
import { RtboxXmlrpcService } from '../shared/rtbox-xmlrpc.service';
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

  constructor(private rs: RtboxXmlrpcService) {}

  ngOnInit(): void {
    this.hostname$ = this.rs.hostname();
    this.serials$ = this.rs.serialInfo();
    this.info$ = this.rs.querySimulation();
  }
}
