import { TestBed } from '@angular/core/testing';

import { RtboxXmlrpcStubService } from './rtbox-xmlrc-stub.service';

describe('RtboxXmlrpcStubService', () => {
  let service: RtboxXmlrpcStubService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(RtboxXmlrpcStubService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});
