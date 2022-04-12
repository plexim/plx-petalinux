import { TestBed } from '@angular/core/testing';

import { RtboxCommunicationStubService } from './rtbox-communication-stub.service';

describe('RtboxCommunicationStubService', () => {
  let service: RtboxCommunicationStubService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(RtboxCommunicationStubService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});
