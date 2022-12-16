# Using NuPetScii charset

The following examples can be assembled into the RC2014 monitor to test NupetScii (graphical) capabilities.

# From RC2014 monitor

The following examples uses the API $02 to display a character on the terminal.

## Normal ASCII display

This example display the character 0x90 on the terminal. This appears as a __REVERSE__ "1".

```
a 8000

ld a, $90 ; Reverse 1 char
ld c, $02 ; API 2 - write char
rst 30    ; API call
ret       ; return to monitor
```

## NuPetScii display

Switch to graphical charset (NuPetScii) with "ESCAPE F" to display the caracter 0x90.

This time, the 0x90 will appears as a partial filled square.

```
a 8000

ld a, $1B ; Switch NuPetSCII
ld c, $02 ;
rst 30    ; API call - Send ESC
ld a, 'F'
rst 30    ; Send F
ld a, $90
rst 30    ; Send char 0x90
ret       ; return to monitor
```

![NupetSCII charset](../nupetscii-font/nupet-ascii-reduced.png)

## Switch back to ASCII

Just send ESC G to switch back to VT100 ASCII charset.

```
a 8000

ld a, $1B ; Switch to ASCII
ld c, $02 ;
rst 30    ; API call - Send ESC
ld a, 'G'
rst 30    ; API call - Send G
```

## Displaying NuPetScii Art

The [nupetscii-font/readme](../nupetscii-font/readme.md) document explains how to use the Playscii software and `psci-extract.py` script to generate NuPetScii art and creates HEX file ready to upload in the SCM (_Small Computer Monitor_).

![NupetSciiDemo](_static/NupetSciiDemo.jpg)

The [NupetSciiDemo.hex](NupetSciiDemo.hex) is available in the /docs folder.

Just open the [NupetSciiDemo.hex](NupetSciiDemo.hex) file and copy/paste the content into the SCM.

```
*m8500                                                                          
8500:  31 0F E0 20 20 20 20 20  20 E0 20 20 20 20 20 20  1..      .             
8510:  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 D5                 .       
8520:  C3 C3 C3 C3 C3 C3 C9 20  20 20 20 20 20 20 20 20  .......                
8530:  20 20 20 E0 CD 20 20 20  20 20 E0 20 20 20 20 20     ..     .            
8540:  20 20 20 20 20 20 20 20  20 20 20 20 C2 20 20 20              .          
8550:  C2 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  .                      
8560:  82 20 20 82 E0 20 CD 20  20 20 20 E0 20 20 20 20  .  .. .    .           
8570:  20 20 20 20 20 20 20 20  20 20 20 20 20 AB C3 C3               ...
```

Once HEX loaded, accordingly to the [nupetscii-font/readme](../nupetscii-font/readme.md) we do have:
* The data loaded at 0x8500.
* The data make 737 bytes length ( 2 bytes + 15 lines of 49 columns continuous data).
* The first bytes is the art Width (49 columns).
* The second bytes is the art Height (15 lines).
* The third byte is the first character of the art (continuous data)

Here follow the assembler for RC2014 SCM used to:
1. Activate the font on the PicoTerm terminal
2. Drawing the screen/art stored at $8500.

```
start:        ; @ $8000
  call init-nupetscii @ $8200
  ld a,($8501); load HEIGHT into c
  ld c,a      
  ld a,($8500); load WIDTH into b
  ld b,a
  ld hl,$8502 ; load data start

loop:       ; @ $800E
  ld a,(hl) ; load char @ hl
  jr nz,display-char ; display now if not null
  ld a, $20 ; replace with space
display-char: @ $8014
  push bc   ; API modifie reg. B & HL
  push hl
  ld c,$02  ; SCM API: output char
  rst 30
  pop hl
  pop bc
  nop
  nop
  nop
  inc hl    ; pointer to next char
  dec b     ; nbre chars (in line)
  jr nz,loop; Jump Relative non-zero @ loop
  push bc   ; API modifie reg. B & HL
  push hl
  ld c, $07 ; SCM API: output carriage return
  rst 30
  pop hl
  pop bc

  dec c     ; nbr lines
  jr z, end ; no more line?
  ld a,($8500); load WIDTH into b
  ld b,a  
  jp loop @ 800E
end: @ $803B
  ret       ; return to SCM

init-nupetscii: @ 8200
  ; send ESC sequence to switch terminal to semi-graphical
  ld a, $1B
  ld c, $02
  rst 30    ; API $02 call - Send ESC
  ld a, 'F'
  rst 30    ; Send F
  ret
```

Here is the de-assembled code from SCM on my RC2014
```
*d 8000                                                                         
8000: CD 00 82     ...   CALL $8200                                             
8003: 3A 01 85     :..   LD A,($8501)                                           
8006: 4F           O     LD C,A                                                 
8007: 3A 00 85     :..   LD A,($8500)                                           
800A: 47           G     LD B,A                                                 
800B: 21 02 85     !..   LD HL,$8502                                            
800E: 7E           ~     LD A,(HL)                                              
800F: 20 03         .    JR NZ,$03  (to $8014)                                  
8011: 00           .     NOP                                                    
8012: 3E 20        >     LD A,$20                                               
8014: C5           .     PUSH BC                                                
8015: E5           .     PUSH HL                                                
8016: 0E 02        ..    LD C,$02                                               
8018: F7           .     RST 30                                                 
8019: E1           .     POP HL                                                 
801A: C1           .     POP BC                                                 
801B: 00           .     NOP                                                    
801C: 00           .     NOP                                                    
801D: 00           .     NOP                                                    
801E: 23           #     INC HL                                                 
801F: 05           .     DEC B                                                  
8020: C2 0E 80     ...   JP NZ,$800E                                            
8023: C5           .     PUSH BC                                                
8024: E5           .     PUSH HL                                                
8025: 0E 07        ..    LD C,$07                                               
8027: F7           .     RST 30                                                 
8028: E1           .     POP HL                                                 
8029: C1           .     POP BC                                                 
802A: 0D           .     DEC C                                                  
802B: CA 3B 80     .;.   JP Z,$803B                                             
802E: 00           .     NOP                                                    
802F: 3A 00 85     :..   LD A,($8500)                                           
8032: 47           G     LD B,A                                                 
8033: C3 0E 80     ...   JP $800E                                               
8036: 00           .     NOP                                                    
8037: 00           .     NOP                                                    
8038: 00           .     NOP                                                    
8039: 00           .     NOP                                                    
803A: 00           .     NOP                                                    
803B: C9           .     RET  


8200: 3E 1B        >.    LD A,$1B                                               
8202: 0E 02        ..    LD C,$02                                               
8204: F7           .     RST 30                                                 
8205: 3E 46        >F    LD A,$46                                               
8207: F7           .     RST 30                                                 
8208: C9           .     RET
```

Once executed here the shiny result on the monitor.

![NupetSciiDemo](_static/NupetSciiDemo-result.jpg)
