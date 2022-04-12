export interface SerialInfo {
  cpu: number;
  mac: string;
  boardRevision: string;
  firmwareVersion: string;
  firmwareBuild: string;
  fpgaVersion: string;
  ipAddresses: string[];
  board: number;
  rtboxType: string;
}

export interface SimulationInfo {
  applicationVersion: string;
  modelName: string;
  modelTimeStamp: number;
  sampleTime: number;
  periodTicks: number[];
  analogInVoltageRange: number;
  analogOutVoltageRange: number;
  digitalOutVoltage: number;
  status: string;
}

export interface Status {
  temperature: number[];
  fanSpeed: number[];
  logPosition: number;
  applicationLog: string;
  clearLog: boolean;
  modelTimeStamp: number;
  voltages: number[];
  currents: number[];
}

export interface PerfCounter {
  maxCycleTime: number[];
  runningCycleTime: number[];
  modelTimeStamp: number;
}

export interface NetState {
  speed: number;
  duplex: string;
  rx_errors: number;
  tx_errors: number;
  rxp: number;
  txp: number;
  collisions: number;
}

export interface Rtbox1LedState {
  error: boolean;
  running: boolean;
  ready: boolean;
  power: boolean;
}

export interface VoltageRange {
  analogIn: number;
  analogOut: number;
  digitalIn: number;
  digitalOut: number;
}

export interface FrontPanelState {
  leds: Rtbox1LedState;
  voltage: VoltageRange;
}
