
# Actividad 3 (grupal) - Equipos e Intrumentación Electrónica
\
**Miembros del grupo**
- Garzón Fernández, **Carlos Enrique**
- Alonso Cabal, **Patricia**
- Díaz Chamorro, **Ginés**

---

\
\
Con esta actividad manejaremos diferentes técnicas para perfeccionar un sistema de medición, control, actuación y presentación del clima en una HMI local mediante supervisión inteligente. 

Concretamente nos centraremos en la implementación de **lógica fuzzy**. Por definición, la lógica difusa (también llamada lógica borrosa (en inglés: fuzzy logic) es una lógica paraconsistente multivaluada en la cual los valores de verdad de las variables pueden ser cualquier número real comprendido entre 0 y 1. La lógica difusa (fuzzy logic, en inglés) permite tomar decisiones más o menos intensas en función de grados intermedios de cumplimiento de una premisa; se adapta mejor al mundo real en el que vivimos, e incluso puede comprender y funcionar con nuestras expresiones, del tipo «hace mucho calor», «no es muy alto», «el ritmo del corazón está un poco acelerado», etc.


\
![Fuzzy Logic](https://media.geeksforgeeks.org/wp-content/uploads/fuzzy-logic_1.png)

\
En este caso la lógica difusa se aplicará al caso de una boya meteorológica. Tras un poco de *research* por parte del equipo, encontramos un paper interesante que nos ha inspirado a aplicar lógica difusa en nuestra boya meteorológica para el estudio de ecosistemas submarinos. Hasta la fecha, la simulación de los habitata acuáticos se había venido desarrollando por medio de modelos basados en curvas de preferencia de las principales variables hidráulicas relacionadas con la calidad de los hábitats. La lógica difusa ofrece varias ventajas frente a implementaciones clásicas. Con ella, se permite introducir un mayor número de variables. Estas, aumentan de forma significativa las posibilidades de combinación entre ellas, lo que nos lleva a incorporar el conocimiento y valoración de expertos de forma efectiva. 

Tras un debate interno, se llegó a la conclusión de que para el hardware al que estamos destinados, lo mejor sería usar la combinación de los siguientes sensores:

- Sensor de distancia para la profundidad del agua.
- Sensor de luz (sensor de luminosidad).
- Sensor de calidad del agua.