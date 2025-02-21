import {
  MonoTypeOperatorFunction,
  ReplaySubject,
  ShareConfig,
  share,
} from "rxjs";

export function isDefined<T>(v: T | null | undefined): v is T {
  return v !== null && v !== undefined;
}

export function shareLastValue<T>(
  config?: Omit<ShareConfig<T>, "connector">
): MonoTypeOperatorFunction<T> {
  return share({ connector: () => new ReplaySubject(1), ...config });
}
