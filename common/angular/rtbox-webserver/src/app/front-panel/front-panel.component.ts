import { Component, OnInit, Inject } from '@angular/core';
import { ViewChild, ElementRef } from '@angular/core';
import { TemplateRef } from '@angular/core';
import { BsModalService, BsModalRef, ModalOptions } from 'ngx-bootstrap/modal';

import { RtboxCgiService } from '../shared/rtbox-cgi.service';
import { RtboxCommunicationService } from '../shared/rtbox-communication.service';
import { Rtbox1LedState, FrontPanelState, SimulationInfo } from '../shared/interfaces';
import { RTBOX_TYPE } from '../shared/rtbox-type';
import { LedLight } from '../lib/labeled-led/labeled-led.component';

class VoltageRanges {
  analogIn = '';
  analogOut = '';
  digitalIn = '';
  digitalOut = '';
}

class IdTriple {
  constructor(public base: string, public hover: string, public detail: ElementRef) {}
}

@Component({
  selector: 'rtb-front-panel',
  templateUrl: './front-panel.component.html',
  styleUrls: ['./front-panel.component.scss']
})
export class FrontPanelComponent implements OnInit {
  LedLight = LedLight;
  errorLed = LedLight.none;
  runningLed = LedLight.none;
  readyLed = LedLight.none;
  powerLed = LedLight.none;

  voltages = new VoltageRanges();
  modalRef: BsModalRef;
  @ViewChild('frontPanelSvg') frontPanelSvg: ElementRef;
  @ViewChild('analogInDetail') analogIn: ElementRef;
  @ViewChild('analogOutDetail') analogOut: ElementRef;
  @ViewChild('digitalInDetail')  digitalIn: ElementRef;
  @ViewChild('digitalOutDetail') digitalOut: ElementRef;
  @ViewChild('resolverInDetail') resolverIn: ElementRef;
  @ViewChild('resolverOutDetail') resolverOut: ElementRef;

  private static voltageRange(index: number): string
  {
      switch (index) {
         case 1: return '-10 V ... 10 V';
         case 2: return '-5 V ... 5 V';
         case 3: return '-2.5 V ... 2.5 V';
         case 4: return '0 ... 10 V';
         case 5: return '0 ... 5 V';
         case 6: return '-2.5V ... 7.5 V';
         case 11: return '5 V';
         case 12: return '3.3 V';
         case 13: return '3.3 V or 5 V';
         default: return '';
      }
  }

  constructor(private cgi: RtboxCgiService,
              private rs: RtboxCommunicationService,
              @Inject(RTBOX_TYPE) public rtboxType: number,
              private modalService: BsModalService) {}

  ngOnInit(): void
  {
    this.refresh();
  }

  refresh(): void
  {
    if (this.rtboxType <= 1) {
      /* The front panel CGI request is currently only implemented on the RTBox 1. Querying it anyhow leads to a 400
       * (bad request), so don't do it in the first place. */
      this.cgi.frontPanel().subscribe(state => this.updateState(state));
    }
    this.rs.querySimulation().subscribe(state => this.updateSimulationInfo(state));
  }

  private updateState(newState: FrontPanelState): void
  {
    this.errorLed = newState.leds.error ? LedLight.red : LedLight.none;
    this.runningLed = newState.leds.running ? LedLight.blue : LedLight.none;
    this.readyLed = newState.leds.ready ? LedLight.green : LedLight.none;
    this.powerLed = newState.leds.power ? LedLight.green : LedLight.none;
  }

  private updateSimulationInfo(newState: SimulationInfo): void
  {
    this.voltages.analogIn = FrontPanelComponent.voltageRange(newState.analogInVoltageRange);
    this.voltages.digitalIn = FrontPanelComponent.voltageRange(13);
    this.voltages.analogOut = FrontPanelComponent.voltageRange(newState.analogOutVoltageRange);
    this.voltages.digitalOut = FrontPanelComponent.voltageRange(newState.digitalOutVoltage);
  }

  showDetails(selector: string): void
  {
     this.showDsubDetail(this[selector] as TemplateRef<any>);
  }

  private showDsubDetail(which: TemplateRef<any>): void
  {
    const config: ModalOptions = { class: 'modal-lg' };

    this.modalRef = this.modalService.show(which, config);
  }
}
