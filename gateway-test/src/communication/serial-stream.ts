import {
  Observable,
  ReplaySubject,
  first,
  share,
  switchMap,
  timer,
} from "rxjs";
import { SerialPort } from "serialport";
import { DataStream } from "./types";
import { shareLastValue } from "../util";

export interface SerialStreamConfig {
  port: string;
  baud: number;
}

export class SerialStream implements DataStream {
  private port$: Observable<SerialPort>;

  constructor(config: SerialStreamConfig) {
    this.port$ = new Observable<SerialPort>((observer) => {
      let expectedClose = false;
      const port = new SerialPort({
        baudRate: config.baud,
        path: config.port,
        autoOpen: false,
        hupcl: false,
      });

      port.open((err) => {
        if (err) observer.error(err);
        else observer.next(port);
      });

      port.once("error", (err) => {
        observer.error(err);
      });

      port.once("close", () => {
        if (expectedClose) {
        } else {
          observer.error(new Error("unexpected port close"));
        }
      });

      return () => {
        expectedClose = true;
        port.close();
      };
    }).pipe(
      shareLastValue({
        resetOnRefCountZero: () => timer(500),
      }),
      share({
        connector: () => new ReplaySubject(1),
      })
    );

    this.rx$ = this.port$.pipe(
      switchMap(
        (p) =>
          new Observable<Buffer>((observer) => {
            const handler = (chunk: Buffer) => {
              observer.next(chunk);
            };
            p.on("data", handler);
            return () => p.off("data", handler);
          })
      ),
      share()
    );
  }

  tx(msg: Buffer): Observable<never> {
    return this.port$.pipe(
      first(),
      switchMap(
        (p) =>
          new Observable<never>((observer) => {
            p.write(msg, (err) => {
              if (err) observer.error(err);
              else observer.complete();
            });
          })
      )
    );
  }

  rx$: Observable<Buffer>;
}
