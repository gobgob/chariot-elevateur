package senpai;

import senpai.Senpai.ErrorCode;
import senpai.comm.CommProtocol.Id;
import senpai.comm.CommProtocol.LLCote;
import senpai.exceptions.ActionneurException;
import senpai.robot.Robot;

/*
 * Copyright (C) 2013-2018 Pierre-François Gimenez
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
 */

/**
 * Montre le périmètre max
 * @author pf
 *
 */

public class DemoPerimetre
{
	public static void main(String[] args)
	{
		String configfile = "demo-perimetre.conf";
		
		Senpai senpai = new Senpai();
		ErrorCode error = ErrorCode.NO_ERROR;
		try
		{
			senpai = new Senpai();
			senpai.initialize(configfile, "default");
			Robot robot = senpai.getService(Robot.class);
			
			robot.updateScore(42);
			// TODO bouge
			while(true)
				Thread.sleep(Integer.MAX_VALUE);
		}
		catch(Exception e)
		{
			Robot robot = senpai.getExistingService(Robot.class);
			error = ErrorCode.EXCEPTION;
			error.setException(e);
		}
		finally
		{
			try
			{
				senpai.destructor(error);
			}
			catch(InterruptedException e)
			{
				e.printStackTrace();
			}
		}
	}
}
