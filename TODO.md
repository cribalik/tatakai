 * Fixa så att animationer alltid börjar på rätt ställe (vilket innebär att animation behöver hålla state och kan inte bara spela utifrån varje Entities tidsstämpel :()
   Man skulle kunna ha animationer som inte är menade att loopa att kunna bara läsa från tidssstämpel, men allt annat blir svårt att göra utan state.
 * Flera spelare
 * Skilj på kollision med väggar och andra entities för optimering
 * 3D support
 * Hur ska referencing och deletion av entities fungera? Som jag ser det har man två alternativ. Vid deletion av en entity kan man swappa med sista, men då måste man fixa referenser genom att ha alla referenser gå genom en extra nivå av indirektion, vilket gör referenskollar långsammare. Andra alternativet är att ha en frilista på entities. Då kommer alltid referenser att fungera, men att iterera genom alla entities kostar extra då vi måste söka genom hela entity buffern istället för den delen som används.
