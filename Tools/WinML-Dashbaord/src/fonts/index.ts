import { initializeIcons as i } from './fabric-icons';

import { IIconOptions } from '@uifabric/styling';
import { registerIconAliases } from './iconAliases';
const DEFAULT_BASE_URL = '';

export function initializeIcons(
  baseUrl: string = DEFAULT_BASE_URL,
  options?: IIconOptions
): void {
  [i].forEach(
    (initialize: (url: string, options?: IIconOptions) => void) => initialize(baseUrl, options)
  );

  registerIconAliases();
}

export { IconNames } from './IconNames';
