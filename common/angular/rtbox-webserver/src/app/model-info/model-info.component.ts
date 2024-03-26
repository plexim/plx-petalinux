import { Component, Input, Inject } from '@angular/core';

import { SimulationInfo } from '../shared/interfaces';
import { RTBOX_TYPE } from '../shared/rtbox-type';

@Component({
  selector: 'rtb-model-info',
  templateUrl: './model-info.component.html',
  styleUrls: ['./model-info.component.scss']
})
export class ModelInfoComponent {
  @Input() info: SimulationInfo = undefined;

  constructor(@Inject(RTBOX_TYPE) public rtboxType: number) {}
}
