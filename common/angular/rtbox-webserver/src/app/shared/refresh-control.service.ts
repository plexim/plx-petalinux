import { Injectable } from '@angular/core';
import { Observable, timer } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class RefreshControlService {
  private readonly refreshMilliSec = 1000;
  private readonly refreshMilliSecCoarse = 5000;
  private timer$: Observable<number>;
  private coarse$: Observable<number>;
  private active = false;

  constructor() {
    this.timer$ = timer(0, this.refreshMilliSec);
    this.coarse$ = timer(0, this.refreshMilliSecCoarse);
  }

  timer(): Observable<number> {
    return this.timer$;
  }

  coarseTimer(): Observable<number> {
    return this.coarse$;
  }
}
