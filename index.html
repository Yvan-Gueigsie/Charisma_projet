<!DOCTYPE html>
<html lang="fr">

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width, initial-scale=1.0">

<title>Suivi de personne</title>

<script src="https://cdn.jsdelivr.net/npm/@tensorflow/tfjs"></script>

<script src="https://cdn.jsdelivr.net/npm/@tensorflow-models/coco-ssd"></script>

<style>

body {
    margin: 0;
    background: black;
    color: white;
    font-family: Arial;
    text-align: center;
}

#container {
    position: relative;
    width: 100%;
    max-width: 800px;
    margin: auto;
}

video {
    width: 100%;
}

canvas {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
}

#info {
    font-size: 20px;
    padding: 15px;
}

</style>

</head>


<body>

<h2>Suivi de personne</h2>

<div id="container">

    <video id="video"
           autoplay
           playsinline>
    </video>

    <canvas id="canvas"></canvas>

</div>

<div id="info">

    x = <span id="x">0</span>

</div>


<script>

const video = document.getElementById("video");

const canvas = document.getElementById("canvas");

const ctx = canvas.getContext("2d");

const xDisplay = document.getElementById("x");


// ======================================================
// IP DE L'ESP32
// ======================================================

const ESP32_IP = "192.168.4.1";


// ======================================================
// DEMARRAGE CAMERA
// ======================================================

async function startCamera()
{

    const stream = await navigator.mediaDevices.getUserMedia({

        video: {
            facingMode: "environment"
        },

        audio: false

    });

    video.srcObject = stream;

}


// ======================================================
// DETECTION PERSONNE
// ======================================================

async function startDetection()
{

    console.log("Chargement du modèle...");

    const model = await cocoSsd.load();

    console.log("Modèle chargé");

    detectPerson(model);

}


// ======================================================
// DETECTION
// ======================================================

async function detectPerson(model)
{

    canvas.width = video.videoWidth;

    canvas.height = video.videoHeight;


    const predictions = await model.detect(video);


    // Nettoyage
    ctx.clearRect(
        0,
        0,
        canvas.width,
        canvas.height
    );


    // Cherche les personnes
    const persons = predictions.filter(
        p => p.class === "person"
    );


    if (persons.length > 0)
    {

        // On prend la personne avec
        // la plus grande probabilité

        persons.sort(
            (a, b) => b.score - a.score
        );

        const person = persons[0];


        // -------------------------------
        // Bounding box
        // -------------------------------

        const [x, y, width, height] =
            person.bbox;


        // Centre de la personne
        const centerX =
            x + width / 2;


        // Centre de l'image
        const imageCenter =
            video.videoWidth / 2;


        // -------------------------------
        // Normalisation
        // -------------------------------

        let normalizedX =
            (centerX - imageCenter)
            / imageCenter;


        // Limitation
        normalizedX =
            Math.max(
                -1,
                Math.min(
                    1,
                    normalizedX
                )
            );


        // Affichage
        xDisplay.textContent =
            normalizedX.toFixed(2);


        // -------------------------------
        // Dessin
        // -------------------------------

        ctx.strokeStyle = "red";

        ctx.lineWidth = 4;

        ctx.strokeRect(
            x,
            y,
            width,
            height
        );


        // Ligne centrale
        ctx.strokeStyle = "yellow";

        ctx.beginPath();

        ctx.moveTo(
            imageCenter,
            0
        );

        ctx.lineTo(
            imageCenter,
            canvas.height
        );

        ctx.stroke();


        // Point central de la personne
        ctx.fillStyle = "lime";

        ctx.beginPath();

        ctx.arc(
            centerX,
            y + height / 2,
            8,
            0,
            2 * Math.PI
        );

        ctx.fill();


        // -------------------------------
        // ENVOI ESP32
        // -------------------------------

        sendPosition(normalizedX);

    }


    // Nouvelle détection
    requestAnimationFrame(
        () => detectPerson(model)
    );

}


// ======================================================
// ENVOI DE X À L'ESP32
// ======================================================

function sendPosition(x)
{

    fetch(
        `http://${ESP32_IP}/position?x=${x.toFixed(3)}`
    )
    .catch(
        error => console.log(
            "ESP32 non connecté"
        )
    );

}


// ======================================================
// DEMARRAGE
// ======================================================

async function start()
{

    await startCamera();

    await startDetection();

}


start();

</script>

</body>

</html>
