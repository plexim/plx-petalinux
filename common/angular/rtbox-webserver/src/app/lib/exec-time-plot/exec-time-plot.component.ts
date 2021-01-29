import { Component, OnInit, Input } from '@angular/core';
import { ExecTime } from '../../processor-load/processor-load.component';

@Component({
  selector: 'rtb-exec-time-plot',
  templateUrl: './exec-time-plot.component.html',
  styleUrls: ['./exec-time-plot.component.scss']
})
export class ExecTimePlotComponent implements OnInit {
  @Input() readonly history: ExecTime[][];
  @Input() readonly minY = 0;
  @Input() readonly maxY = 100;
  @Input() readonly minX = 0;
  @Input() readonly maxX = 100;
  @Input() readonly width = 400;
  @Input() readonly height = 200;
  @Input() readonly nXTics = 5;
  @Input() readonly nYTics = 5;
  @Input() readonly ticLength = 6;
  @Input() readonly yAxisLabel = 'y';
  @Input() readonly showCores = [true, true, true];
  @Input() readonly showMax = false;

  readonly pl = 40;
  readonly pr = 10;
  readonly pt = 8;
  readonly pb = 42;

  readonly xLabelOffset = 8;
  readonly yLabelOffset = 18;

  xTicDelta: number;

  xTics = new Array<number>();
  yTics = new Array<number>();

  constructor() {}

  coreIndices(): number[] {
    return this.history.map((_, index) => index);
  }

  polylineCurrent(core: number): string {
    return this.polyline(core, t => t.current);
  }

  polygonCurrent(core: number): string {
    const y = this.height - this.pb;
    const x1 = this.pl;
    const x2 = this.pl + (this.history[core].length - 1) * this.xTicDelta;
    const path = `${x1},${y} ${this.polylineCurrent(core)} ${x2},${y}`;

    return path;
  }

  polylineMax(core: number): string {
    return this.polyline(core, t => t.maximum);
  }

  private polyline(core: number, select: any): string {
    const nHist = this.history[core].length;
    let path = '';

    for (let i = 0; i < nHist; ++i) {
      const t = this.history[core][i];
      const x = this.pl + i * this.xTicDelta;
      const y = this.height - this.pb - select(t) / this.maxY * (this.height - this.pt - this.pb);

      path += `${x},${y} `;
    }

    return path;
  }

  ngOnInit(): void {
    this.xTicDelta = (this.width - this.pl - this.pr) / (this.maxX - this.minX);

    const yTicDelta = (this.height  - this.pb - this.pt) / (this.nYTics - 1);
    const xTicDelta = (this.width  - this.pl - this.pr) / (this.nXTics - 1);

    for (let i = 0; i < this.nYTics; ++i) {
      this.yTics.push(this.height - this.pb - i * yTicDelta);
    }

    for (let i = 0; i < this.nXTics; ++i) {
      this.xTics.push(this.pl + i * xTicDelta);
    }
  }
}
