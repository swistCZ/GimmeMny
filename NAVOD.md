# Návod k obsluze QR Code Presenteru

Toto zařízení slouží k jednoduchému generování platebních QR kódů (standard SPAYD) pro předem nastavený bankovní účet. Obsahuje také funkci sčítací kalkulačky.

## Ovládání

### Zadávání částky a kalkulačka
- **Číslice `0-9`**: Zadávání aktuální částky. Částka se zadává v celých korunách (např. pro 123 Kč zadejte `123`).
- **Tlačítko `*` (hvězdička)**: Smaže poslední zadanou číslici (funkce Backspace).
- **Tlačítko `A`**: Přičte aktuálně zadanou částku k mezisoučtu (funkce **`+`**). Na displeji se zobrazí mezisoučet a můžete zadat další položku.
- **Tlačítko `B`**: Zobrazí konečný součet všech položek (funkce **`=`**). Výsledek zůstane na displeji.
- **Tlačítko `C`**: Vynuluje celou kalkulaci i zadanou částku (funkce **Reset**).

### Zobrazení QR kódu
- **Tlačítko `#` (mřížka)**: Vygeneruje a zobrazí QR kód pro aktuálně zobrazenou částku (buď právě zadanou, nebo výsledek kalkulace).
  - **Displej se otočí o 180 stupňů**, abyste ho mohli snadno ukázat platící osobě.

### Návrat zpět
- **Tlačítko `D`**: Pokud je zobrazen QR kód, tímto tlačítkem se vrátíte zpět na obrazovku pro zadávání částky.

## Konfigurace
Číslo bankovního účtu a další parametry lze změnit v souboru `data/config.ini`. Po úpravě je nutné nahrát souborový systém do zařízení pomocí příkazu:
```bash
pio run -t uploadfs
```
