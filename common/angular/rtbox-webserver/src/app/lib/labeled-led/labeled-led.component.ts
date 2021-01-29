import { Component, OnInit, Input } from '@angular/core';

export enum LedLight {
  none,
  red,
  green,
  blue
}

@Component({
  selector: 'rtb-labeled-led',
  templateUrl: './labeled-led.component.html',
  styleUrls: ['./labeled-led.component.scss']
})
export class LabeledLedComponent implements OnInit {
  LedLight = LedLight;
  @Input() color = LedLight.none;

  /* &nbsp; that works with Angular. We want to allow Leds without a label, but a completely empty string results in the
   * CSS magic to not be rendered at all, hence the default space: */
  @Input() text = '\xa0';

  constructor() {}

  ngOnInit(): void {}
}
