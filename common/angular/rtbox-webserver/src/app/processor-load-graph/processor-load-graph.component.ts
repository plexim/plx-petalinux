import { Component, Input, Inject, OnInit } from '@angular/core';
import { ExecTime } from '../processor-load/processor-load.component';
import { RTBOX_TYPE } from '../shared/rtbox-type';

@Component({
  selector: 'rtb-processor-load-graph',
  templateUrl: './processor-load-graph.component.html',
  styleUrls: ['./processor-load-graph.component.scss']
})
export class ProcessorLoadGraphComponent implements OnInit {
  @Input() readonly history: ExecTime[][];
  @Input() readonly histLength: number;
  @Input() readonly width: number;
  @Input() readonly height: number;
  coreVisibility = [true, true, true];
  maxVisibility = this.rtboxType <= 1 ? true : false;

  constructor(@Inject(RTBOX_TYPE) public rtboxType: number) {}

  ngOnInit(): void {}

  toggleShowCore(core: number): void {
    this.coreVisibility[core] = !this.coreVisibility[core];

    this.coreVisibility = [].concat(this.coreVisibility);
  }
}
