# Configuration d'OSMV ⚙️

OSMV est extrêmement flexible. Les paramètres sont gérés via l'interface graphique QML et stockés dans le fichier `settings.json`. Le daemon d'arrière-plan surveille ce fichier et applique immédiatement les modifications sans redémarrage.

---

## 📺 Intégration OBS Studio

Le widget de rendu d'OSMV utilise des technologies web standards (HTML/CSS/JS) pour afficher la musique de façon moderne et fluide avec un effet de glassmorphism.

### Procédure de configuration :
1. Dans OBS Studio, ajoutez une nouvelle source **Navigateur** (Browser Source).
2. Cochez la case **Fichier local**.
3. Cliquez sur **Parcourir** et sélectionnez le fichier `shared/index.html` de votre dossier OSMV.
4. Spécifiez les dimensions :
   - **Largeur : 500**
   - **Hauteur : 140**
5. (Optionnel) Cochez **Actualiser le navigateur lorsque la scène devient active** pour vous assurer qu'il se synchronise instantanément si nécessaire.

*Note : Les styles CSS par défaut se trouvent dans `shared/style.css`. Vous pouvez éditer ce fichier pour modifier les animations, les polices ou les couleurs.*

---

## 🎮 Discord Rich Presence (RPC)

Le module Discord affiche votre statut d'écoute en temps réel dans votre profil Discord.

### Paramètres disponibles :
- **Discord Rich Presence (ON / OFF)** : Active ou désactive totalement l'intégration Discord.
- **Utiliser la musique si disponible (ON / OFF)** : 
  - Si **ON** (par défaut), votre statut affichera le titre de la chanson, l'artiste et l'album en cours de lecture.
  - Si **OFF** (ou si aucune musique n'est détectée), le statut affichera vos textes personnalisés.
- **Client ID de l'application** : ID d'application obtenu sur le [Portail des développeurs Discord](https://discord.com/developers/applications). Requis pour afficher des images personnalisées.
- **Détails & État personnalisés** : Textes affichés lorsque aucune musique ne tourne ou que l'option de suivi musical est désactivée.
- **Grandes et petites images** : Clés d'images téléversées dans la section *Rich Presence Art Assets* de votre application Discord.
  - Par défaut, les clés pour les badges de statut de lecture sont : `playing`, `paused`, `stopped`.

---

## ⏰ Module Horloge (Time)

Le module de temps d'OSMV permet d'écrire l'heure système locale dans un fichier texte à la seconde près. C'est l'équivalent moderne du logiciel Snaz.

### Configuration du format :
Vous pouvez personnaliser la chaîne de caractères à l'aide des balises dynamiques suivantes :

| Balise | Description | Exemple |
| :--- | :--- | :--- |
| `$h` | Heure (12h ou 24h selon l'option) | `15` ou `03` |
| `$m` | Minutes | `42` |
| `$s` | Secondes | `09` |
| `$tt` | Indicateur AM / PM | `PM` (uniquement si 12 heures est coché) |

### Exemple de formats :
- `$h:$m:$s` ➔ `15:42:09`
- `$h heures, $m minutes` ➔ `15 heures, 42 minutes`
- `$h:$m:$s $tt` (avec format 12h activé) ➔ `03:42:09 PM`

### Sortie texte :
Le fichier de sortie est écrit dans `shared/current_time.txt`. Vous pouvez ajouter une source **Texte (GDI+)** dans OBS et cocher **Lire depuis un fichier** pour l'intégrer sur votre scène de stream.
