
import { SerialInfo, SimulationInfo, PerfCounter } from './interfaces';
import { FrontPanelState, VoltageRange, Rtbox1LedState } from './interfaces';

interface QueryPerfCounterRtbox1 {
  maxCycleTime: number;
  runningCycleTime: number | string;
  runningLatency: number | string;
  modelTimeStamp: number;
}

interface QueryPerfCounterRtbox23 {
  maxCycleTime1: number | string;
  maxCycleTime2: number | string;
  maxCycleTime3: number | string;
  modelTimeStamp: number;
  runningCycleTime1: number | string;
  runningCycleTime2: number | string;
  runningCycleTime3: number | string;
}

function isPerfCounterRtbox23(response: QueryPerfCounterRtbox1 | QueryPerfCounterRtbox23):
  response is QueryPerfCounterRtbox23
{
  return (response as QueryPerfCounterRtbox23).maxCycleTime3 !== undefined;
}

export class ResponseConversion {
  static adjustSampleTime<T>(from: any): T
  {
    return {
      ...from,
      sampleTime: from.sampleTime === '' ? undefined : from.sampleTime * 1.e9
    };
  }

  private static toTime(value: number | string): number
  {
    if (typeof value === typeof 0) {
      return value as number;
    } else {
      return 0;
    }
  }

  private static toTimeArray(values: (number | string)[]): number[]
  {
    return values.map(ResponseConversion.toTime);
  }

  static perfCounter(from: QueryPerfCounterRtbox1 | QueryPerfCounterRtbox23): PerfCounter
  {
    const counter: PerfCounter = {
      modelTimeStamp: from.modelTimeStamp,
      maxCycleTime: [],
      runningCycleTime: [],
    };

    if (isPerfCounterRtbox23(from)) {
      const from23 = from as QueryPerfCounterRtbox23;
      counter.maxCycleTime = ResponseConversion.toTimeArray([
        from23.maxCycleTime1, from23.maxCycleTime2, from23.maxCycleTime3]);
      counter.runningCycleTime = ResponseConversion.toTimeArray([
        from23.runningCycleTime1, from23.runningCycleTime2, from23.runningCycleTime3]);
    }
    else {
      const from1 = from as QueryPerfCounterRtbox1;
      counter.maxCycleTime = [ ResponseConversion.toTime(from1.maxCycleTime) ];
      counter.runningCycleTime = [ ResponseConversion.toTime(from1.runningCycleTime) ];
    }

    return counter;
  }

  static frontPanelState(state: any): FrontPanelState {
    const result: FrontPanelState = {
      leds: {
        error: state.leds.error === '1' ? true : false,
        running: state.leds.running === '1' ? true : false,
        ready: state.leds.ready === '1' ? true : false,
        power: state.leds.power === '1' ? true : false
      },
      voltage: state.voltageRanges
    };

    return result;
  }
}
