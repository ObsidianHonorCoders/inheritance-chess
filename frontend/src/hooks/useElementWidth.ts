import { useLayoutEffect, useRef, useState } from 'react';

/** Tracks the content-box width of an element via ResizeObserver. */
export function useElementWidth<T extends HTMLElement>() {
  const ref = useRef<T>(null);
  const [width, setWidth] = useState(0);

  useLayoutEffect(() => {
    const element = ref.current;
    if (!element) return;

    setWidth(Math.floor(element.getBoundingClientRect().width));
    const observer = new ResizeObserver((entries) => {
      const entry = entries[0];
      if (entry) setWidth(Math.floor(entry.contentRect.width));
    });
    observer.observe(element);
    return () => observer.disconnect();
  }, []);

  return [ref, width] as const;
}
