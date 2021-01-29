import { Component, OnInit, Input } from '@angular/core';

@Component({
  selector: 'rtb-arc-gauge',
  templateUrl: './arc-gauge.component.html',
  styleUrls: ['./arc-gauge.component.scss']
})
export class ArcGaugeComponent implements OnInit {

  @Input() values: number[][];
  @Input() min = 0;
  @Input() max = 100;
  @Input() width = 300;
  @Input() height = 200;
  @Input() arcWidth = 10;

  ngOnInit(): void {
  }

  getViewBox(): string {
    return '0 0 ' + this.width + ' ' + this.height;
  }

  getCX(): number {
    return 0.5 * this.width;
  }

  getCY(): number {
    return 0.5 * this.width;
  }

  getPath(value0: number, value1: number, idx: number): string {
    if (value0 > value1) {
      return '';
    }
    const alpha0 = (1 - (value0 - this.min) / (this.max - this.min)) * Math.PI;
    const alpha1 = (1 - (value1 - this.min) / (this.max - this.min)) * Math.PI;
    const r = 0.4 * this.width - 1.5 * idx * this.arcWidth;
    const cX = this.getCX();
    const cY = this.getCY();
    const x0 = cX + r * Math.cos(alpha0);
    const y0 = cY - r * Math.sin(alpha0);
    const x1 = cX + r * Math.cos(alpha1);
    const y1 = cY - r * Math.sin(alpha1);

    let path = 'M' + x0 + ',' + y0 + ' ';
    path += 'A' + r + ',' + r + ' 0 0 1 ' + x1 + ',' + y1;
    return path;
  }
}
