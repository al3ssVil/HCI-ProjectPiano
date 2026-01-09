import { Component, OnInit, NgZone } from '@angular/core';
import { NgFor } from '@angular/common';

interface Note {
  index: number;
  name: string;
}

interface Melody {
  name: string;
  notes: number[];
}

@Component({
  selector: 'app-piano',
  templateUrl: './piano.html',
  styleUrls: ['./piano.css'],
  imports: [NgFor]
})
export class Piano implements OnInit {

  notes: Note[] = [
    { index: 0, name: 'C4' }, { index: 1, name: 'C#4' }, { index: 2, name: 'D4' }, { index: 3, name: 'D#4' },
    { index: 4, name: 'E4' }, { index: 5, name: 'F4' }, { index: 6, name: 'F#4' }, { index: 7, name: 'G4' },
    { index: 8, name: 'G#4' }, { index: 9, name: 'A4' }, { index: 10, name: 'A#4' }, { index: 11, name: 'B4' }
  ];

  activeNotes: number[] = [];
  highlightedNote: number | null = null; // next note to press in melody
  mode: 'free' | 'melody' = 'free';

  melodies: Melody[] = [
    { name: 'Twinkle Twinkle', notes: [0, 0, 7, 7, 9, 9, 7] },
    { name: 'Mary Had a Little Lamb', notes: [4, 2, 0, 2, 4, 4, 4] },
    { name: 'Ode to Joy', notes: [4, 4, 5, 7, 7, 5, 4, 2, 0] },
    { name: 'Happy Birthday', notes: [0, 0, 2, 0, 5, 4] },
    { name: 'Jingle Bells', notes: [4, 4, 4, 4, 4, 4, 4, 0, 2, 4, 5, 4] }
  ];

  currentMelody: number[] = [];
  currentStep: number = 0;

  constructor(private zone: NgZone) { }

  ngOnInit(): void {
    const evtSource = new EventSource('http://172.17.38.226/sse');

    evtSource.onopen = () => console.log('SSE connected');

    evtSource.onmessage = (event) => {
      const [type, valueStr] = event.data.split(':');
      const value = parseInt(valueStr);

      this.zone.run(() => {
        if (type === 'note_on') {
          this.handleNote(value);
        }
      });
    };

    evtSource.onerror = (err) => console.error('SSE error', err);
  }

  handleNote(noteIndex: number) {
    if (noteIndex === -1) {
      // eliberezi tasta
      this.activeNotes = [];
      return;
    }

    this.activeNotes = [noteIndex]; // galben cât ții apăsat

    if (this.mode === 'melody') {
      const expectedNote = this.currentMelody[this.currentStep];
      if (noteIndex === expectedNote) {
        this.currentStep++;
        this.updateHighlightedNote();

        if (this.currentStep >= this.currentMelody.length) {
          setTimeout(() => {
            alert('Melody completed! Congrats!');
            this.resetMelody();
          }, 500);
        }
      } else {
        setTimeout(() => {
          alert('Wrong note! Start again.');
          this.resetMelody();
        }, 200);
      }
    }
  }

  updateHighlightedNote() {
    this.highlightedNote = this.currentStep < this.currentMelody.length
      ? this.currentMelody[this.currentStep]
      : null;
  }

  resetMelody() {
    this.activeNotes = [];
    this.currentStep = 0;
    if (this.mode === 'melody' && this.currentMelody.length > 0) {
      this.activeNotes = [];
      this.updateHighlightedNote();
    }
    else {
      this.highlightedNote = null;
    }
  }

  startMelody(notes: number[]) {
    this.mode = 'melody';
    this.currentMelody = notes;
    this.currentStep = 0;
    this.activeNotes = [];
    this.updateHighlightedNote();
  }

  selectMelody(event: Event) {
    const select = event.target as HTMLSelectElement;
    const index = select.selectedIndex - 1; // first option is placeholder
    if (index >= 0) {
      this.startMelody(this.melodies[index].notes);
    }
  }

  switchToFreeMode() {
    this.mode = 'free';
    this.resetMelody();
    const selectEl = document.querySelector('select') as HTMLSelectElement | null;
    if (selectEl) {
      selectEl.selectedIndex = 0;
    }
  }

  isActive(index: number): boolean {
    return this.activeNotes.includes(index);
  }

  isHighlighted(index: number): boolean {
    return this.highlightedNote === index;
  }
}
