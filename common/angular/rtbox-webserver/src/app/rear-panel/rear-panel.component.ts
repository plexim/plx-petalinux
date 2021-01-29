import { Component, OnInit, Inject } from '@angular/core';
import { ViewChild, ElementRef } from '@angular/core';
import { BsModalService, BsModalRef, ModalOptions } from 'ngx-bootstrap/modal';

import { RTBOX_TYPE } from '../shared/rtbox-type';

@Component({
  selector: 'rtb-rear-panel',
  templateUrl: './rear-panel.component.html',
  styleUrls: ['./rear-panel.component.scss']
})
export class RearPanelComponent implements OnInit {
  @ViewChild('ethDetail') eth: ElementRef;
  @ViewChild('canDetail') can: ElementRef;
  @ViewChild('rsDetail') rs: ElementRef;
  modalRef: BsModalRef;

  constructor(@Inject(RTBOX_TYPE) public rtboxType: number, private modalService: BsModalService) {}

  ngOnInit(): void {}

  showDetails(selector: string): void
  {
     this.showDsubDetail(this[selector] as ElementRef);
  }

  private showDsubDetail(which: ElementRef): void
  {
    const config: ModalOptions = { class: 'modal-lg' };

    this.modalRef = this.modalService.show(which, config);
  }
}
