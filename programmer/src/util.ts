import {
  EMPTY,
  Observable,
  OperatorFunction,
  Subject,
  concatMap,
  defer,
  filter,
  first,
  from,
  ignoreElements,
  scan,
  share,
  timeout,
  timer,
} from "rxjs";
import { SerialPort } from "serialport";
import { readFile } from "node:fs";

export function log(message: string) {
  return defer(() => {
    console.log(message);
    return EMPTY;
  });
}

export function openPort(portPath: string) {
  return new Observable<{
    tx: (data: string) => Observable<never>;
    waitForValue: (value: string, timeoutMsec: number) => Observable<never>;
  }>((observer) => {
    const port = new SerialPort({
      path: portPath,
      autoOpen: false,
      baudRate: 57600,
      stopBits: 1,
      dataBits: 8,
      parity: "none",
    });

    const rx$ = new Subject<Buffer>();

    const sharedRx$ = rx$.pipe(
      share({
        resetOnRefCountZero: () => timer(100),
      })
    );

    port.on("data", (data) => {
      rx$.next(data);
    });

    port.on("open", () => {
      observer.next({
        tx: (data) =>
          new Observable<never>((observer) => {
            port.write(data, (error) => {
              if (error) {
                observer.error(error);
              } else {
                observer.complete();
              }
            });
          }),
        waitForValue(value: string, timeoutMsec: number) {
          return sharedRx$.pipe(waitFor(value, timeoutMsec));
        },
      });
    });

    port.on("error", (error) => {
      observer.error(error);
    });

    port.open();

    return () => port.close();
  });
}

export function readFileLines(filePath: string) {
  return new Observable<Buffer>((observer) => {
    readFile(filePath, (error, data) => {
      if (error) {
        observer.error(error);
      } else {
        observer.next(data);
        observer.complete();
      }
    });
  }).pipe(
    concatMap((data) => {
      const fileLines = data
        .toString()
        .split("\n")
        .filter((line) => !!line);

      return from(fileLines);
    }),
    filter((line) => {
      if (!line.startsWith(":")) return false;
      const recLen = +line.substring(1, 3);
      const recType = +line.substring(7, 9);

      if (recLen == 0 && recType != 0)
        // Valid data record?
        return false;

      return true;
    })
  );
}

export function waitFor(
  value: string,
  timeoutMsec: number
): OperatorFunction<Buffer, never> {
  const msg = Buffer.from(value);
  return (src) =>
    defer(() =>
      src.pipe(
        scan(
          (ctx, chunk) => {
            let pending = Buffer.concat([ctx.pending, chunk]);
            ctx.send = [];

            if (pending.indexOf(msg) >= 0) {
              ctx.send.push(Buffer.from("found"));
            }

            ctx.pending = pending;

            return ctx;
          },
          {
            pending: Buffer.from([]),
            send: [] as Buffer[],
          }
        ),
        concatMap((v) => v.send),
        first(),
        timeout(timeoutMsec),
        ignoreElements()
      )
    );
}
