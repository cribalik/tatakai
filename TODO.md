 * Rendera text!
 * 3D support
 * Skilj på om man går in i en hitBox eller ut ur en
 * Hur ska referencing och deletion av entities fungera? Som jag ser det har man två alternativ. Vid deletion av en entity kan man swappa med sista, men då måste man fixa referenser genom att ha alla referenser gå genom en extra nivå av indirektion, vilket gör referenskollar långsammare. Andra alternativet är att ha en frilista på entities. Då kommer alltid referenser att fungera, men att iterera genom alla entities kostar extra då vi måste söka genom hela entity buffern istället för den delen som används.
 * Om man står på marken och krockar med flera saker så kan man sjunka genom golvet. Verkar som att hoppa över en sak möjliggör att man hoppar över flera saker. Det kanske är t som sätts fel, eller fel testobject som tas bort?
