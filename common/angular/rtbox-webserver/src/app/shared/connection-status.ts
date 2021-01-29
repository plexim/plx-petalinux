
import { InjectionToken } from '@angular/core';
import { BehaviorSubject } from 'rxjs';

export const CONNECTION_STATUS = new InjectionToken<BehaviorSubject<string>>('CONNECTION_STATUS');

