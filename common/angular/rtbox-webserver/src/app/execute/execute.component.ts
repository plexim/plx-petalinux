import { Component, OnInit, ElementRef, ViewChild } from '@angular/core';
import { mergeMap, tap } from 'rxjs/operators';
import { RtboxCgiService } from '../shared/rtbox-cgi.service';
import { RtboxCommunicationService } from '../shared/rtbox-communication.service';
import { RefreshControlService } from '../shared/refresh-control.service';
import { TemplateRef } from '@angular/core';
import { BsModalService, BsModalRef, ModalOptions } from 'ngx-bootstrap/modal';
import { timer, interval } from 'rxjs';
import { takeUntil } from 'rxjs/operators';
import { HttpErrorResponse } from '@angular/common/http';

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
  currentErrorMsg = '';
  secondsUntilReload: number = undefined;
  fileBackground: string = 'neutral';
  private readonly modalConfig: ModalOptions = { class: 'modal-sm' };
  @ViewChild('modalError') modalError: TemplateRef<any>;
  @ViewChild('rebootWait') rebootWait: TemplateRef<any>;
  @ViewChild('fileToUploadInput') fileToUploadInput: ElementRef<HTMLInputElement>;

  private readonly noop = () => undefined;

  constructor(private cgi: RtboxCgiService,
              private rs: RtboxCommunicationService,
              private refresh: RefreshControlService,
              private modalService: BsModalService) {}

  ngOnInit(): void {}

  uploadFile(event: MouseEvent): void
  {
    this.canStop = false;

    const upload = () => this.cgi.upload(this.fileToUpload, this.startAfterUpload, this.delayUntilFirstTrigger);

    this.cgi.stopExecution().pipe(this.tapEnableCanStop(), mergeMap(upload)).subscribe(
      response => this.handleUploadSuccess(response),
      error => this.handleUploadError(error));
  }

  private tapEnableCanStop() {
    return tap(() => this.canStop = true);
  }

  fileInputChange(files: FileList): void
  {
    this.fileToUpload = files.item(0);
    this.fileBackground = 'neutral';
  }

  start(event: MouseEvent): void
  {
    this.cgi.startExecution(this.delayUntilFirstTrigger).subscribe(
      response => this.handleStartSuccess(response),
      error => this.handleStartError(error));
  }

  private handleStartSuccess(response: any): void
  {
    this.currentErrorMsg = '';
  }

  private handleStartError(error: any): void
  {
    if (error.status >= 400) {
      this.currentErrorMsg = error.error;

      this.modalRef = this.modalService.show(this.modalError, this.modalConfig);
    }
  }

  stop(event: MouseEvent): void
  {
    this.canStop = false;

    const stop$ = this.cgi.stopExecution();

    stop$.pipe(this.tapEnableCanStop()).subscribe(this.noop, console.error);
  }

  handleUploadSuccess(message: any): void {
    this.currentErrorMsg = '';
    this.fileBackground = 'success'
  }

  handleUploadError(error: HttpErrorResponse): void {
    if (error.status >= 400) {
      this.currentErrorMsg = error.error;
      this.fileBackground = 'error';
    }
    else {
      // handle the case where a file cannot be read by the browser after it was
      // changed. See https://gitlab.plexim.com/hil/hil_sw/-/issues/5574
      const reader = new FileReader();
      reader.onerror = (
        error => {
          this.currentErrorMsg = error.target.error.message;
          this.fileBackground = 'neutral';
          this.fileToUpload = null;
          this.fileToUploadInput.nativeElement.value = '';
        }
      );
      // The attempt to read the file will trigger the error which is 
      // handled above.
      reader.readAsArrayBuffer(this.fileToUpload);
    }

    this.modalRef = this.modalService.show(this.modalError, this.modalConfig)
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
