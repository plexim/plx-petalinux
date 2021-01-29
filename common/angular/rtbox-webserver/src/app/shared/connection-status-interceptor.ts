
import { Inject, Injectable, InjectionToken } from '@angular/core';
import { HttpEvent, HttpInterceptor, HttpHandler, HttpRequest, HttpResponse } from '@angular/common/http';

import { Observable, BehaviorSubject } from 'rxjs';
import { tap } from 'rxjs/operators';

import { CONNECTION_STATUS } from './connection-status';

@Injectable()
export class ConnectionStatusInterceptor implements HttpInterceptor {
  private static connectionMsg(status: number): string
  {
    if (status < 100) {
      return 'Connection lost';
    } else if (status >= 500) {
      return `Server error (${status})`;
    } else if (status >= 400) {
      return `Client error (${status})`;
    }

    return '';
  }

  constructor(@Inject(CONNECTION_STATUS) private status: BehaviorSubject<string>) {
    this.status.next('No connection');
  }

  intercept(request: HttpRequest<any>, next: HttpHandler): Observable<HttpEvent<any>>
  {
    const broadcastStatus = this.updateConnectionStatus.bind(this);

    return next.handle(request).pipe(
      tap(broadcastStatus));
  }

  private updateConnectionStatus(event: HttpEvent<any>): void
  {
    if (event instanceof HttpResponse) {
      const status = (event as HttpResponse<any>).status;
      const message = ConnectionStatusInterceptor.connectionMsg(status);

      this.status.next(message);
    }
  }
}
