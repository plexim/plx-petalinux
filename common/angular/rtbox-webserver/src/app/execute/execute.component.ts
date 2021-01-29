import { Component, OnInit, ElementRef, ViewChild } from '@angular/core';
import { flatMap, tap } from 'rxjs/operators';
import { RtboxCgiService } from '../shared/rtbox-cgi.service';
import { RtboxXmlrpcService } from '../shared/rtbox-xmlrpc.service';
import { RefreshControlService } from '../shared/refresh-control.service';
import { TemplateRef } from '@angular/core';
import { BsModalService, BsModalRef, ModalOptions } from 'ngx-bootstrap/modal';
import { timer, interval } from 'rxjs';
import { takeUntil } from 'rxjs/operators';

@Component({
  selector: 'rtb-execute',
  templateUrl: './execute.component.html',
  styleUrls: ['./execute.component.scss']
})
export class ExecuteComponent implements OnInit {
  fileToUpload: File = null;
  startAfterUpload = true;
  delayUntilFirstTrigger = false;
  canStop = true;
  canReboot = true;
  modalRef: BsModalRef;
  startSimulationErrorMsg = '';
  secondsUntilReload: number = undefined;
  private readonly modalConfig: ModalOptions = { class: 'modal-sm' };
  @ViewChild('modalError') modalError: ElementRef;
  @ViewChild('rebootWait') rebootWait: ElementRef;

  private readonly noop = () => undefined;

  constructor(private cgi: RtboxCgiService,
              private rs: RtboxXmlrpcService,
              private refresh: RefreshControlService,
              private modalService: BsModalService) {}

  ngOnInit(): void {}

  uploadFile(event: MouseEvent): void
  {
    this.canStop = false;

    const upload = () => this.cgi.upload(this.fileToUpload, this.startAfterUpload, this.delayUntilFirstTrigger);

    this.cgi.stopExecution().pipe(this.tapEnableCanStop(), flatMap(upload)).subscribe(this.noop, console.error);
  }

  private tapEnableCanStop() {
    return tap(() => this.canStop = true);
  }

  fileInputChange(files: FileList): void
  {
    this.fileToUpload = files.item(0);
  }

  start(event: MouseEvent): void
  {
    this.cgi.startExecution(this.delayUntilFirstTrigger).subscribe(
      response => this.handleStartSuccess(response),
      error => this.handleStartError(error));
  }

  private handleStartSuccess(response: any): void
  {
    this.startSimulationErrorMsg = '';
  }

  private handleStartError(error: any): void
  {
    if (error.status === 500) {
      this.startSimulationErrorMsg = error.error;

      this.modalRef = this.modalService.show(this.modalError, this.modalConfig);
    }
  }

  stop(event: MouseEvent): void
  {
    this.canStop = false;

    const stop$ = this.cgi.stopExecution();

    stop$.pipe(this.tapEnableCanStop()).subscribe(this.noop, console.error);
  }

  reboot(): void {
    this.canReboot = false;
    this.modalRef.hide();

    this.secondsUntilReload = 25;
    const reload$ = timer(this.secondsUntilReload * 1000);

    interval(1000).pipe(takeUntil(reload$)).subscribe(t => --this.secondsUntilReload);

    reload$.subscribe(t => this.reloadPage());

    this.modalRef = this.modalService.show(this.rebootWait, this.modalConfig);

    const noop = () => undefined;
    this.rs.reboot().subscribe(noop, noop);
  }

  reloadPage(): void {
    location.reload();
  }

  openModal(template: TemplateRef<any>) {
    this.modalRef = this.modalService.show(template, this.modalConfig);
  }
}
