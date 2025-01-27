import {
  concat,
  concatMap,
  defer,
  first,
  ignoreElements,
  merge,
  of,
  race,
  throwError,
} from "rxjs";
import { log, openPort, readFileLines } from "./util";

const fileName = process.argv[2];
const portPath = process.argv[3];

openPort(portPath)
  .pipe(
    concatMap(({ waitForValue, tx }) =>
      concat(
        waitForValue(" UNO CC Debug Ready ", 5000),
        merge(waitForValue("$$$", 250), tx("@")),
        log("Target Ready!"),
        merge(waitForValue(" ERASE CHIP ?:", 250), tx("e")),
        merge(waitForValue("Done", 250), tx("Y")),
        log("Chip Erased!"),
        readFileLines(fileName).pipe(
          concatMap((line) =>
            concat(
              merge(waitForValue(":", 250), tx(":")),
              defer(() => {
                const rxSuccess$ = concat(
                  waitForValue("*\r\n", 5000),
                  of("ok")
                );
                const rxError$ = concat(
                  waitForValue("\r\nE*CS:", 5000),
                  throwError(() => new Error(`Error writing line ${line}`))
                );

                return concat(
                  log(`write ${line}`),
                  merge(race(rxSuccess$, rxError$), tx(line.substring(1)))
                );
              })
            )
          ),
          ignoreElements()
        ),
        merge(waitForValue("RESET", 250), tx("R")),
        log(`Target reset!`),
        of("complete")
      )
    ),
    first()
  )
  .subscribe({
    complete: () => {
      console.log("Complete");
    },
    error: (error) => {
      console.error(error);
    },
  });
