import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

# Daten laden
df = pd.read_csv('move_times.csv')

# Plot erstellen
plt.figure(figsize=(10, 6))
sns.boxplot(x='player', y='move_time', data=df)

# Optik verbessern
plt.title('Rechenzeit pro Zug pro Agent')
plt.xlabel('Spieler / Agent ID')
plt.ylabel('Zeit in Sekunden')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Speichern oder Anzeigen
plt.savefig('move_time_boxplot.png')
plt.show()