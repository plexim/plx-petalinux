import { Component, Input } from '@angular/core';
import { BsModalRef } from 'ngx-bootstrap/modal';

@Component({
  selector: 'rtb-modal-detail-table',
  templateUrl: './modal-detail-table.component.html',
  styleUrls: ['./modal-detail-table.component.scss']
})
export class ModalDetailTableComponent {
  @Input() modal: BsModalRef;
  @Input() title = '';

  constructor() {}
}
