import { Component } from '@angular/core';
import { Piano } from './piano/piano';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [Piano], 
  template: `
    <h1 style="text-align:center">ESP32 Piano</h1>
    <app-piano></app-piano>
  `,
})
export class App { }
