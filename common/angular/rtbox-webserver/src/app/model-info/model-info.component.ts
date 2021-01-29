import { Component, Input } from '@angular/core';

import { SimulationInfo } from '../shared/interfaces';

@Component({
  selector: 'rtb-model-info',
  templateUrl: './model-info.component.html',
  styleUrls: ['./model-info.component.scss']
})
export class ModelInfoComponent {
  @Input() info: SimulationInfo = undefined;

  constructor() {}
}
