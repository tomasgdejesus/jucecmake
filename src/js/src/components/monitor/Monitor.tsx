import { useRef, useEffect } from 'react';

type MonitorProps = {
  data: number[]
};

/// TODO: FIX THIS

function Monitor({data}: MonitorProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    if (!canvasRef.current) return;

    const ctx = canvasRef.current.getContext('2d');
    if (!ctx) return;

    ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    ctx.fillStyle = 'black';
    ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);

    ctx.beginPath();
    ctx.strokeStyle = 'white';
    ctx.lineWidth = 1;
    ctx.moveTo(0, ctx.canvas.height/2);
    ctx.lineTo(ctx.canvas.width, ctx.canvas.height/2);
    ctx.stroke();

    ctx.beginPath();
    ctx.strokeStyle = 'lime';
    ctx.lineWidth = 1;

    ctx.moveTo(0, 0);

    for (let i = 0; i < data.length; i++) {
      const xpos = i * (ctx.canvas.width/data.length);
      const ypos = Math.floor((((-data[i] + 1) / 2) * ctx.canvas.height));

      if (i == 0)
        ctx.moveTo(xpos, ypos);
      else
        ctx.lineTo(xpos, ypos);
    }

    ctx.stroke();
  }, [data]);

  return <canvas ref={canvasRef} style={{width:'80%', height:'100px'}}></canvas>;
}

export default Monitor;